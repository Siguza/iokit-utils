#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <mach/mach.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>

#define LOG(str, args...) do { printf(str "\n", ##args); } while(0)

int main(int argc, const char **argv)
{
    if(argc < 2)
    {
        LOG("Usage: %s ClassName", argv[0]);
        return 1;
    }

    CFStringRef class = CFStringCreateWithCStringNoCopy(NULL, argv[1], kCFStringEncodingUTF8, kCFAllocatorNull);
    for(int i = 0; class != NULL; ++i)
    {
        LOG("%*s%s", i, "", CFStringGetCStringPtr(class, kCFStringEncodingUTF8));
        CFStringRef super = IOObjectCopySuperclassForClass(class);
        CFRelease(class);
        class = super;
    }

    return 0;
}
