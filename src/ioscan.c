#include <stdbool.h>            // bool, true, false
#include <stdint.h>             // uint32_t, uint64_t
#include <stdio.h>              // printf
#include <stdlib.h>             // strtol

#include <mach/kern_return.h>   // kern_return_t, KERN_SUCCESS
#include <mach/mach_error.h>    // mach_error_string
#include <mach/mach_host.h>     // mach_port_t
#include <mach/mach_traps.h>    // mach_host_self
#include <mach/port.h>          // MACH_PORT_NULL, MACH_PORT_VALID

#include <IOKit/IOKitLib.h>     // IO*, io_*, kIO*

#define LOG(str, args...) printf(str, ##args)

int main(int argc, const char **argv)
{
    LOG("%-40s%-40s%-64s %-6s %-6s %s\n", "Class", "Name", "Spawn", "one", "two", "equal");
    io_iterator_t it;
    IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching(argc >= 2 ? argv[1] : "IOService"), &it);
    io_object_t o;
    int type = argc >= 3 ? (int)strtol(argv[2], NULL, 0) : 0;
    while((o = IOIteratorNext(it)) != 0)
    {
        kern_return_t ret = 0;

        io_name_t name;
        IORegistryEntryGetName(o, name);

        io_connect_t one = MACH_PORT_NULL,
                     two = MACH_PORT_NULL;
        ret = IOServiceOpen(o, mach_task_self(), type, &one);
        if(ret == KERN_SUCCESS && MACH_PORT_VALID(one))
        {
            IOServiceOpen(o, mach_task_self(), type, &two);
        }
        LOG("%-40s%-40s%-64s %-6u %-6u %s\n", CFStringGetCStringPtr(IOObjectCopyClass(o), kCFStringEncodingUTF8), name, mach_error_string(ret), one, two, two == 0 ? "" : one == two ? "==" : "!=");

        if(one) IOServiceClose(one);
        if(two) IOServiceClose(two);
    }
    return 0;
}
