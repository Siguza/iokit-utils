#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#include "common.h"

int main(int argc, const char **argv)
{
    bool bundle = false;
    int aoff;
    for(aoff = 1; aoff < argc; ++aoff)
    {
        if(argv[aoff][0] != '-')
        {
            break;
        }
        if(strcmp(argv[aoff], "-b") == 0)
        {
            bundle = true;
        }
    }

    if(argc - aoff < 1)
    {
        LOG("Usage: %s [-b] ClassName", argv[0]);
        return 1;
    }

    CFStringRef class = CFStringCreateWithCStringNoCopy(NULL, argv[aoff], kCFStringEncodingUTF8, kCFAllocatorNull);
    if(bundle)
    {
        CFStringRef str = IOObjectCopyBundleIdentifierForClass(class);
        if(str)
        {
            LOG("%s", CFStringGetCStringPtr(str, kCFStringEncodingUTF8));
        }
        else
        {
            LOG(COLOR_RED "Class not found" COLOR_RESET);
        }
    }
    else
    {
        for(int i = 0; class != NULL; ++i)
        {
            LOG("%*s%s", i, "", CFStringGetCStringPtr(class, kCFStringEncodingUTF8));
            CFStringRef super = IOObjectCopySuperclassForClass(class);
            CFRelease(class);
            class = super;
        }
    }

    return 0;
}
