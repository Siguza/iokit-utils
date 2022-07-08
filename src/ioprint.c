/* Copyright (c) 2017-2022 Siguza
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This Source Code Form is "Incompatible With Secondary Licenses", as
 * defined by the Mozilla Public License, v. 2.0.
**/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>

#include "cfj.h"
#include "common.h"
#include "iokit.h"

static bool printEntry(io_object_t o, const char *match, bool hdr, bool xml, bool cfj, bool json, bool set)
{
    static CFDictionaryRef dict = NULL;
    if(set && dict == NULL)
    {
        CFStringRef key = CFSTR("herp");
        CFStringRef val = CFSTR("derp");
        dict = CFDictionaryCreate(NULL, (const void**)&key, (const void**)&val, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        if(dict == NULL)
        {
            ERR(COLOR_RED "Failed to create dict" COLOR_RESET);
            return false;
        }
    }

    io_name_t name;
    kern_return_t ret = IORegistryEntryGetName(o, name);
    if(ret != KERN_SUCCESS)
    {
        ERR(COLOR_RED "IORegistryEntryGetName: %s" COLOR_RESET, mach_error_string(ret));
        return false;
    }
    if(!match || IOObjectConformsTo(o, match) || strcmp(name, match) == 0)
    {
        io_name_t class;
        ret = _IOObjectGetClass(o, kIOClassNameOverrideNone, class);
        if(ret != KERN_SUCCESS)
        {
            ERR(COLOR_RED "class(%s): %s" COLOR_RESET, name, mach_error_string(ret));
            return false;
        }

        if(set)
        {
            kern_return_t ret = IORegistryEntrySetCFProperties(o, dict);
            if(hdr)
            {
                LOG("%s%s(%s):%s %s%s%s",
                    COLOR_CYAN, class, name, COLOR_RESET,
                    ret == KERN_SUCCESS ? COLOR_GREEN : COLOR_YELLOW, mach_error_string(ret), COLOR_RESET
                );
            }
        }
        if(xml || cfj || json)
        {
            CFMutableDictionaryRef p = NULL;
            kern_return_t ret = IORegistryEntryCreateCFProperties(o, &p, NULL, 0);
            if(hdr && !set)
            {
                LOG("%s%s(%s):%s %s%s%s",
                    COLOR_CYAN, class, name, COLOR_RESET,
                    ret == KERN_SUCCESS ? COLOR_GREEN : COLOR_YELLOW, mach_error_string(ret), COLOR_RESET
                );
            }
            if(ret == KERN_SUCCESS)
            {
                if(xml)
                {
                    CFDataRef prop = CFPropertyListCreateData(NULL, p, kCFPropertyListXMLFormat_v1_0, 0, NULL);
                    if(prop)
                    {
                        LOG("%.*s", (int)CFDataGetLength(prop), CFDataGetBytePtr(prop));
                        CFRelease(prop);
                    }
                    else
                    {
                        CFShow(p);
                    }
                }
                if(cfj)
                {
                    cfj_print(stdout, p, false, true);
                }
                if(json)
                {
                    cfj_print(stdout, p, true, false);
                }
                CFRelease(p);
            }
        }
        else if(hdr && !set)
        {
            LOG("%s%s(%s)%s", COLOR_CYAN, class, name, COLOR_RESET);
        }
    }
    return true;
}

static void print_help(const char *self)
{
    fprintf(stderr, "Usage:\n"
                    "    %s [options] [name]\n"
                    "\n"
                    "Description:\n"
                    "    Iterate over all registry entries and optionally perform some operations.\n"
                    "    If name is given, only entries with matching class or instance name are considered.\n"
                    "\n"
                    "Options:\n"
                    "    -d          Print IOKit properties in XML format\n"
                    "    -h          Print this help and exit\n"
                    "    -j          Print IOKit properties in JSON format\n"
                    "    -k          Print IOKit properties in mix between JSON and hexdump\n"
                    "    -o          Print only IOKit properties and nothing else\n"
                    "    -p plane    Iterate over the given registry plane (default: IOService)\n"
                    "    -s          Try to set the entries' properties\n"
           , self
    );
}

int main(int argc, const char **argv)
{
    bool hdr  = true,
         xml  = false,
         cfj  = false,
         json = false,
         set  = false;
    const char *plane = "IOService";
    int aoff;
    for(aoff = 1; aoff < argc; ++aoff)
    {
        if(argv[aoff][0] != '-')
        {
            break;
        }
        bool opt = true;
        for(size_t i = 1; opt; ++i)
        {
            char c = argv[aoff][i];
            if(c == '\0')
            {
                break;
            }
            switch(c)
            {
                case 'h':
                    print_help(argv[0]);
                    return -1;

                case 'd':
                    xml = true;
                    break;

                case 'j':
                    json = true;
                    break;

                case 'k':
                    cfj = true;
                    break;

                case 'o':
                    hdr = false;
                    break;

                case 's':
                    set = true;
                    break;

                case 'p':
                    if(argv[aoff][i+1] != '\0' || ++aoff >= argc)
                    {
                        ERR(COLOR_RED "Missing argument to -p" COLOR_RESET);
                        printf("\n");
                        print_help(argv[0]);
                        return -1;
                    }
                    plane = argv[aoff];
                    opt = false;
                    break;

                default:
                    ERR(COLOR_RED "Unrecognized argument: %s" COLOR_RESET, argv[aoff]);
                    printf("\n");
                    print_help(argv[0]);
                    return -1;
            }
        }
    }

    const char *match = aoff < argc ? argv[aoff] : NULL;
    io_object_t o = IORegistryGetRootEntry(kIOMasterPortDefault);
    bool succ = printEntry(o, match, hdr, xml, cfj, json, set);
    IOObjectRelease(o);
    if(!succ)
    {
        return -1;
    }

    int retval = 0;
    io_iterator_t it = MACH_PORT_NULL;
    if(IORegistryCreateIterator(kIOMasterPortDefault, plane, kIORegistryIterateRecursively, &it) == KERN_SUCCESS)
    {
        while((o = IOIteratorNext(it)) != 0)
        {
            succ = printEntry(o, match, hdr, xml, cfj, json, set);
            IOObjectRelease(o);
            if(!succ)
            {
                retval = -1;
                break;
            }
        }
        IOObjectRelease(it);
    }
    return retval;
}
