#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>

#include "cfj.h"
#include "common.h"
#include "iokit.h"

static bool printEntry(io_object_t o, const char *match, bool xml, bool json, bool set)
{
    static CFDictionaryRef dict = NULL;
    if(set && dict == NULL)
    {
        CFStringRef key = CFSTR("herp");
        CFStringRef val = CFSTR("derp");
        dict = CFDictionaryCreate(NULL, (const void**)&key, (const void**)&val, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        if(dict == NULL)
        {
            LOG(COLOR_RED "Failed to create dict" COLOR_RESET);
            return false;
        }
    }

    io_name_t name;
    kern_return_t ret = IORegistryEntryGetName(o, name);
    if(ret != KERN_SUCCESS)
    {
        LOG(COLOR_RED "IORegistryEntryGetName: %s" COLOR_RESET, mach_error_string(ret));
        return false;
    }
    if(!match || IOObjectConformsTo(o, match) || strcmp(name, match) == 0)
    {
        io_name_t class;
        ret = IOObjectGetClass(o, class);
        if(ret != KERN_SUCCESS)
        {
            LOG(COLOR_RED "class(%s): %s" COLOR_RESET, name, mach_error_string(ret));
            return false;
        }

        if(set)
        {
            kern_return_t ret = IORegistryEntrySetCFProperties(o, dict);
            LOG("%s%s(%s):%s %s%s%s",
                COLOR_CYAN, class, name, COLOR_RESET,
                ret == KERN_SUCCESS ? COLOR_GREEN : COLOR_YELLOW, mach_error_string(ret), COLOR_RESET
            );
        }
        else
        {
            if(xml || json)
            {
                CFMutableDictionaryRef p = NULL;
                kern_return_t ret = IORegistryEntryCreateCFProperties(o, &p, NULL, 0);
                LOG("%s%s(%s):%s %s%s%s",
                    COLOR_CYAN, class, name, COLOR_RESET,
                    ret == KERN_SUCCESS ? COLOR_GREEN : COLOR_YELLOW, mach_error_string(ret), COLOR_RESET
                );
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
                    if(json)
                    {
                        cfj_print(stdout, p);
                    }
                    CFRelease(p);
                }
            }
            else
            {
                LOG("%s%s(%s)%s", COLOR_CYAN, class, name, COLOR_RESET);
            }
        }
    }
    return true;
}

static void print_help(const char *self)
{
    printf("Usage:\n"
           "    %s [options] [name]\n"
           "\n"
           "Description:\n"
           "    Iterate over all registry entries and optionally perform some operations.\n"
           "    If name is given, only entries with matching class or instance name are considered.\n"
           "\n"
           "Options:\n"
           "    -d          Dump (print) the entries' properties\n"
           "    -h          Print this help and exit\n"
           "    -j          Print properties in JSON-like format\n"
           "    -p plane    Iterate over the given registry plane (default: IOService)\n"
           "    -s          Try to set the entries' properties\n"
           , self
    );
}

int main(int argc, const char **argv)
{
    bool xml  = false,
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
        else if(strcmp(argv[aoff], "-h") == 0)
        {
            print_help(argv[0]);
            return 0;
        }
        else if(strcmp(argv[aoff], "-d") == 0)
        {
            xml  = true;
        }
        else if(strcmp(argv[aoff], "-j") == 0)
        {
            json = true;
        }
        else if(strcmp(argv[aoff], "-p") == 0)
        {
            ++aoff;
            if(aoff >= argc)
            {
                LOG(COLOR_RED "Missing argument to -p" COLOR_RESET);
                printf("\n");
                print_help(argv[0]);
                return 1;
            }
            plane = argv[aoff];
        }
        else if(strcmp(argv[aoff], "-s") == 0)
        {
            set = true;
        }
        else
        {
            LOG(COLOR_RED "Unrecognized argument: %s" COLOR_RESET, argv[aoff]);
            printf("\n");
            print_help(argv[0]);
            return 1;
        }
    }

    const char *match = aoff < argc ? argv[aoff] : NULL;
    io_object_t o = IORegistryGetRootEntry(kIOMasterPortDefault);
    bool succ = printEntry(o, match, xml, json, set);
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
            succ = printEntry(o, match, xml, json, set);
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
