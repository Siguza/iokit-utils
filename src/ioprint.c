#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#define LOG(str, args...) do { printf(str "\n", ##args); } while(0)

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
            LOG("Failed to create dict");
            exit(1);
        }
    }

    io_name_t name;
    IORegistryEntryGetName(o, name);
    if(!match || IOObjectConformsTo(o, match) || strcmp(name, match) == 0)
    {
        CFStringRef class = IOObjectCopyClass(o);
        const char *className = CFStringGetCStringPtr(class, kCFStringEncodingUTF8);
        if(set)
        {
            kern_return_t ret = IORegistryEntrySetCFProperties(o, dict);
            LOG("\x1b[1;93m%s(%s): %s (0x%x)\x1b[0m", className, name, mach_error_string(ret), ret);
        }
        else
        {
            if(dump)
            {
                CFMutableDictionaryRef p = NULL;
                kern_return_t ret = IORegistryEntryCreateCFProperties(o, &p, NULL, 0);
                LOG("\x1b[1;93m%s(%s): %s (0x%x)\x1b[0m", className, name, mach_error_string(ret), ret);
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
                LOG("\x1b[1;93m%s(%s)\x1b[0m", className, name);
            }
        }
        CFRelease(class);
    }
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
        if(strcmp(argv[aoff], "-d") == 0)
        {
            dump = true;
        }
        else if(strcmp(argv[aoff], "-p") == 0)
        {
            ++aoff;
            if(aoff >= argc)
            {
                LOG("Missing argument to -p");
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
            LOG("Unrecognized argument: %s", argv[aoff]);
            return 1;
        }
    }

    const char *match = aoff < argc ? argv[aoff] : NULL;
    io_object_t o = IORegistryGetRootEntry(kIOMasterPortDefault);
    printEntry(o, match, dump, set);
    IOObjectRelease(o);

    io_iterator_t it;
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
