#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach/mach.h>

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#include "common.h"

static void printEntry(io_object_t o, const char *match, bool dump, bool set)
{
    static CFDictionaryRef dict = NULL;
    if(set && dict == NULL)
    {
        CFStringRef key = CFSTR("herp");
        CFStringRef val = CFSTR("derp");
        CFDictionaryRef dict = CFDictionaryCreate(NULL, (const void**)&key, (const void**)&val, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        if(dict == NULL)
        {
            LOG(COLOR_RED "Failed to create dict" COLOR_RESET);
            exit(1);
        }
    }

    io_name_t name;
    kern_return_t ret = IORegistryEntryGetName(o, name);
    if(ret != KERN_SUCCESS)
    {
        LOG(COLOR_RED "IORegistryEntryGetName: %s" COLOR_RESET, mach_error_string(ret));
        exit(1);
    }
    if(!match || IOObjectConformsTo(o, match) || strcmp(name, match) == 0)
    {
        CFStringRef class = IOObjectCopyClass(o);
        if(!class)
        {
            LOG(COLOR_RED "IOObjectCopyClass(%s): %s" COLOR_RESET, name, mach_error_string(ret));
            exit(1);
        }

        const char *className = CFStringGetCStringPtr(class, kCFStringEncodingUTF8);
        if(set)
        {
            kern_return_t ret = IORegistryEntrySetCFProperties(o, dict);
            LOG("%s%s(%s):%s %s%s%s",
                COLOR_CYAN, className, name, COLOR_RESET,
                ret == KERN_SUCCESS ? COLOR_GREEN : COLOR_YELLOW, mach_error_string(ret), COLOR_RESET
            );
        }
        else
        {
            if(dump)
            {
                CFMutableDictionaryRef p = NULL;
                kern_return_t ret = IORegistryEntryCreateCFProperties(o, &p, NULL, 0);
                LOG("%s%s(%s):%s %s%s%s",
                    COLOR_CYAN, className, name, COLOR_RESET,
                    ret == KERN_SUCCESS ? COLOR_GREEN : COLOR_YELLOW, mach_error_string(ret), COLOR_RESET
                );
                if(ret == KERN_SUCCESS)
                {
                    CFDataRef xml = CFPropertyListCreateData(NULL, p, kCFPropertyListXMLFormat_v1_0, 0, NULL);
                    if(xml)
                    {
                        LOG("%.*s", (int)CFDataGetLength(xml), CFDataGetBytePtr(xml));
                        CFRelease(xml);
                    }
                    else
                    {
                        CFShow(p);
                    }
                    CFRelease(p);
                }
            }
            else
            {
                LOG("%s%s(%s)%s", COLOR_CYAN, className, name, COLOR_RESET);
            }
        }
        CFRelease(class);
    }
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
           "    -p plane    Iterate over the given registry plane (default: IOService)\n"
           "    -s          Try to set the entries' properties\n"
           , self
    );
}

int main(int argc, const char **argv)
{
    bool dump  = false,
         set    = false;
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
            dump = true;
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
    printEntry(o, match, dump, set);
    IOObjectRelease(o);

    io_iterator_t it = MACH_PORT_NULL;
    if(IORegistryCreateIterator(kIOMasterPortDefault, plane, kIORegistryIterateRecursively, &it) == KERN_SUCCESS)
    {
        while((o = IOIteratorNext(it)) != 0)
        {
            printEntry(o, match, dump, set);
            IOObjectRelease(o);
        }
        IOObjectRelease(it);
    }
    return 0;
}
