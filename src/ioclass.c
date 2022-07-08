/* Copyright (c) 2017-2018 Siguza
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
#include <string.h>
#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>

#include "common.h"
#include "iokit.h"

int main(int argc, const char **argv)
{
    bool bundle  = false,
         extends = false;
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
        else if(strcmp(argv[aoff], "-e") == 0)
        {
            extends = true;
        }
        else
        {
            ERR(COLOR_RED "Unrecognized argument: %s" COLOR_RESET, argv[aoff]);
            return -1;
        }
    }

    if(argc - aoff < 1)
    {
        ERR("Usage: %s [-b] [-e] ClassName", argv[0]);
        return -1;
    }

    CFStringRef class = CFStringCreateWithCStringNoCopy(NULL, argv[aoff], kCFStringEncodingUTF8, kCFAllocatorNull);
    if(extends)
    {
        io_registry_entry_t root = IORegistryGetRootEntry(kIOMasterPortDefault);
        CFDictionaryRef diag = IORegistryEntryCreateCFProperty(root, CFSTR("IOKitDiagnostics"), NULL, 0);
        CFDictionaryRef classes = CFDictionaryGetValue(diag, CFSTR("Classes"));
        CFIndex num = CFDictionaryGetCount(classes);
        CFStringRef *names = malloc(num * sizeof(CFStringRef));
        CFDictionaryGetKeysAndValues(classes, (const void**)names, NULL);

        for(size_t i = 0; i < num; ++i)
        {
            CFStringRef actual = names[i];
            CFStringRef current = actual;
            CFRetain(current);
            while(current != NULL)
            {
                if(CFEqual(current, class))
                {
                    if(bundle)
                    {
                        CFStringRef bndl = IOObjectCopyBundleIdentifierForClass(actual);
                        LOG(COLOR_CYAN "%s" COLOR_RESET " (%s)", CFStringGetCStringPtr(actual, kCFStringEncodingUTF8), CFStringGetCStringPtr(bndl, kCFStringEncodingUTF8));
                        CFRelease(bndl);
                    }
                    else
                    {
                        LOG(COLOR_CYAN "%s" COLOR_RESET, CFStringGetCStringPtr(actual, kCFStringEncodingUTF8));
                    }
                    CFRelease(current);
                    break;
                }
                CFStringRef super = IOObjectCopySuperclassForClass(current);
                CFRelease(current);
                current = super;
            }
        }

        free(names);
        CFRelease(diag);
        IOObjectRelease(root);
        CFRelease(class);
    }
    else if(bundle)
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
        CFRelease(str);
        CFRelease(class);
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
