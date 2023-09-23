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
                    char classStr[512];
                    if(!CFStringGetCString(actual, classStr, sizeof(classStr), kCFStringEncodingUTF8))
                    {
                        ERR(COLOR_RED "Failed to convert class name to UTF-8." COLOR_RESET);
                        return -1;
                    }
                    if(bundle)
                    {
                        CFStringRef bndl = IOObjectCopyBundleIdentifierForClass(actual);
                        char bundleStr[512];
                        if(!CFStringGetCString(bndl, bundleStr, sizeof(bundleStr), kCFStringEncodingUTF8))
                        {
                            ERR(COLOR_RED "Failed to convert bundle name to UTF-8." COLOR_RESET);
                            return -1;
                        }
                        LOG("%s (%s)", classStr, bundleStr);
                        CFRelease(bndl);
                    }
                    else
                    {
                        LOG("%s", classStr);
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
        CFStringRef bndl = IOObjectCopyBundleIdentifierForClass(class);
        if(bndl)
        {
            char bundleStr[512];
            if(!CFStringGetCString(bndl, bundleStr, sizeof(bundleStr), kCFStringEncodingUTF8))
            {
                ERR(COLOR_RED "Failed to convert bundle name to UTF-8." COLOR_RESET);
                return -1;
            }
            LOG("%s", bundleStr);
        }
        else
        {
            LOG(COLOR_RED "Class not found" COLOR_RESET);
        }
        CFRelease(bndl);
        CFRelease(class);
    }
    else
    {
        for(int i = 0; class != NULL; ++i)
        {
            char classStr[512];
            if(!CFStringGetCString(class, classStr, sizeof(classStr), kCFStringEncodingUTF8))
            {
                ERR(COLOR_RED "Failed to convert class name to UTF-8." COLOR_RESET);
                return -1;
            }
            LOG("%*s%s", i, "", classStr);
            CFStringRef super = IOObjectCopySuperclassForClass(class);
            CFRelease(class);
            class = super;
        }
    }

    return 0;
}
