#include <errno.h>              // errno
#include <math.h>               // floor, log2
#include <stdbool.h>            // bool, true, false
#include <stdint.h>             // uint32_t, uint64_t
#include <stdlib.h>             // strtol, malloc
#include <string.h>             // strerror, strlcpy

#include <mach/kern_return.h>   // kern_return_t, KERN_SUCCESS
#include <mach/mach_error.h>    // mach_error_string
#include <mach/mach_host.h>     // mach_port_t
#include <mach/mach_traps.h>    // mach_host_self
#include <mach/port.h>          // MACH_PORT_NULL, MACH_PORT_VALID

#include "common.h"
#include "iokit.h"

typedef struct ioscan
{
    struct ioscan *next;
    io_name_t class;
    io_name_t ucClass;
    io_name_t name;
    uint32_t type;
    kern_return_t spawn;
    io_connect_t one;
    io_connect_t two;
} ioscan_t;

static ioscan_t** processEntry(io_object_t o, const char *plane, const char *match, uint32_t min, uint32_t max, bool only_success, ioscan_t **ptr)
{
    io_name_t name;
    kern_return_t ret = IORegistryEntryGetName(o, name);
    if(ret != KERN_SUCCESS)
    {
        name[0] = '\0';
    }
    if(!match || IOObjectConformsTo(o, match) || (name[0] && strcmp(name, match) == 0))
    {
        io_name_t class;
        ret = _IOObjectGetClass(o, kIOClassNameOverrideNone, class);
        if(ret != KERN_SUCCESS)
        {
            class[0] = '\0';
        }
        for(uint32_t i = min; i <= max; ++i)
        {
            io_connect_t one = MACH_PORT_NULL,
                         two = MACH_PORT_NULL;
            ret = IOServiceOpen(o, mach_task_self(), i, &one);
            if(ret == KERN_SUCCESS && MACH_PORT_VALID(one))
            {
                IOServiceOpen(o, mach_task_self(), i, &two);
            }

            if(!only_success || ret == KERN_SUCCESS)
            {
                ioscan_t *data = malloc(sizeof(ioscan_t));
                if(!data)
                {
                    LOG(COLOR_RED "Failed to allocate entry for %s: %s" COLOR_RESET, name, strerror(errno));
                    return NULL;
                }
                data->next = NULL;
                data->class[0] = '\0';
                data->ucClass[0] = '\0';
                data->name[0] = '\0';
                data->type = i;
                data->spawn = ret;
                data->one = one;
                data->two = two;

                strlcpy(data->name, name, sizeof(io_name_t));
                strlcpy(data->class, class, sizeof(io_name_t));

                if(ret == KERN_SUCCESS && MACH_PORT_VALID(one))
                {
                    io_iterator_t it = MACH_PORT_NULL;
                    if(IORegistryEntryGetChildIterator(o, plane, &it) == KERN_SUCCESS)
                    {
                        io_object_t client = MACH_PORT_NULL;
                        while((client = IOIteratorNext(it)) != 0)
                        {
                            io_struct_inband_t buf;
                            uint32_t len = sizeof(buf);
                            ret = IORegistryEntryGetProperty(client, "IOUserClientCreator", buf, &len);
                            if(ret == KERN_SUCCESS)
                            {
                                uint32_t pid;
                                if(sscanf(buf, "pid %u,", &pid) == 1)
                                {
                                    if(pid == getpid())
                                    {
                                        io_name_t ucClass;
                                        ret = _IOObjectGetClass(client, kIOClassNameOverrideNone, ucClass);
                                        if(ret == KERN_SUCCESS)
                                        {
                                            strlcpy(data->ucClass, ucClass, sizeof(io_name_t));
                                        }
                                        IOObjectRelease(client);
                                        break;
                                    }
                                }
                            }
                            IOObjectRelease(client);
                        }
                        IOObjectRelease(it);
                    }
                }

                *ptr = data;
                ptr = &data->next;
            }

            if(one) IOServiceClose(one);
            if(two) IOServiceClose(two);
        }
    }
    return ptr;
}

static void print_help(const char *self)
{
    printf("Usage:\n"
           "    %s [options] [name [min [max]]]\n"
           "\n"
           "Description:\n"
           "    Iterate over all registry entries and try to spawn UserClients.\n"
           "    If name is given, only entries with matching class or instance name are considered.\n"
           "    If min and max are given, all types in between are tried.\n"
           "    If only min is given, only that type is tried, otherwise it defaults to type 0.\n"
           "\n"
           "Options:\n"
           "    -h          Print this help and exit\n"
           "    -p plane    Iterate over the given registry plane (default: IOService)\n"
           "    -s          Print only successful spawning attempts\n"
           , self
    );
}

int main(int argc, const char **argv)
{
    bool only_success = false;
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
            return -1;
        }
        else if(strcmp(argv[aoff], "-p") == 0)
        {
            ++aoff;
            if(aoff >= argc)
            {
                LOG(COLOR_RED "Missing argument to -p" COLOR_RESET);
                printf("\n");
                print_help(argv[0]);
                return -1;
            }
            plane = argv[aoff];
        }
        else if(strcmp(argv[aoff], "-s") == 0)
        {
            only_success = true;
        }
        else
        {
            LOG(COLOR_RED "Unrecognized argument: %s" COLOR_RESET, argv[aoff]);
            printf("\n");
            print_help(argv[0]);
            return -1;
        }
    }

    const char *match = NULL;
    uint32_t min = 0,
             max = 0;
    if(aoff < argc)
    {
        match = argv[aoff];
        ++aoff;
        if(aoff < argc)
        {
            min = max = (uint32_t)strtol(argv[aoff], NULL, 0);
            ++aoff;
            if(aoff < argc)
            {
                max = (uint32_t)strtol(argv[aoff], NULL, 0);
                ++aoff;
            }
        }
    }

    // Need to get all entries here, because spawning clients invalidates our iterator
    size_t num = 1024,
           idx = 0;
    io_object_t *objs = malloc(num * sizeof(io_object_t));
    if(!objs)
    {
        LOG(COLOR_RED "Failed to allocate objects buffer: %s" COLOR_RESET, strerror(errno));
        return -1;
    }

    objs[idx++] = IORegistryGetRootEntry(kIOMasterPortDefault);
    io_iterator_t it = MACH_PORT_NULL;
    if(IORegistryCreateIterator(kIOMasterPortDefault, plane, kIORegistryIterateRecursively, &it) == KERN_SUCCESS)
    {
        io_object_t o;
        while((o = IOIteratorNext(it)) != 0)
        {
            if(idx >= num)
            {
                num *= 2;
                objs = realloc(objs, num * sizeof(io_object_t));
                if(!objs)
                {
                    LOG(COLOR_RED "Failed to reallocate objects buffer: %s" COLOR_RESET, strerror(errno));
                    return -1;
                }
            }
            objs[idx++] = o;
        }
        IOObjectRelease(it);
    }

    ioscan_t *head = NULL,
             **ptr = &head;
    for(size_t i = 0; i < idx; ++i)
    {
        ptr = processEntry(objs[i], plane, match, min, max, only_success, ptr);
        if(!ptr)
        {
            for(; i < idx; ++i)
            {
                IOObjectRelease(objs[i]);
            }
            free(objs);
            objs = NULL;
            for(ioscan_t *node = head; node != NULL; )
            {
                ioscan_t *next = node->next;
                free(node);
                node = next;
            }
            return -1;
        }
        IOObjectRelease(objs[i]);
    }
    free(objs);
    objs = NULL;

    int classLen = strlen("Class"),
        nameLen  = strlen("Name"),
        typeLen  = strlen("Type"),
        spawnLen = strlen("Spawn"),
        ucLen    = strlen("UC"),
        oneLen   = strlen("One"),
        twoLen   = strlen("Two"),
        equalLen = strlen("Equal");

    for(ioscan_t *node = head; node != NULL; node = node->next)
    {
        int l  = strlen(node->class[0] ? node->class : "failed");
        if(l > classLen) classLen = l;
        l = strlen(node->name);
        if(l == 0)
        {
            l = strlen("failed");
        }
        if(l > nameLen) nameLen = l;
        l = 1 + (node->type == 0 ? 0 : (int)floor(log10(node->type))); // Decimal
        if(l > typeLen) typeLen = l;
        l = strlen(mach_error_string(node->spawn));
        if(l > spawnLen) spawnLen = l;
        l = strlen(node->ucClass);
        if(l > ucLen) ucLen = l;
        l = 1 + (node->one == 0 ? 0 : (int)floor(log2(node->one) / 4)); // Hex
        if(l > oneLen) oneLen = l;
        l = 1 + (node->two == 0 ? 0 : (int)floor(log2(node->two) / 4)); // Hex
        if(l > twoLen) twoLen = l;
    }

    LOG(COLOR_CYAN "%-*s %-*s %*s %-*s %-*s %*s %*s %-*s" COLOR_RESET,
        classLen, "Class",
        nameLen,  "Name",
        typeLen,  "Type",
        spawnLen, "Spawn",
        ucLen,    "UC",
        oneLen,   "One",
        twoLen,   "Two",
        equalLen, "Equal"
    );
    for(ioscan_t *node = head; node != NULL; )
    {
        ioscan_t *next = node->next;
        LOG("%s%-*s%s %s%-*s%s %s%*u%s %s%-*s%s %s%-*s%s %*x %*x %-*s",
            node->class[0] ? "" : COLOR_RED, classLen, node->class[0] ? node->class : "failed", node->class[0] ? "" : COLOR_RESET,
            node->name[0]  ? "" : COLOR_RED, nameLen,  node->name[0]  ? node->name  : "failed", node->name[0]  ? "" : COLOR_RESET,
            COLOR_PURPLE, typeLen, node->type, COLOR_RESET,
            node->spawn == KERN_SUCCESS ? COLOR_GREEN : COLOR_YELLOW, spawnLen, mach_error_string(node->spawn), COLOR_RESET,
            COLOR_BLUE, ucLen, node->ucClass, COLOR_RESET,
            oneLen, node->one,
            twoLen, node->two,
            equalLen, node->two == 0 ? "" : node->one == node->two ? "==" : "!=");
        free(node);
        node = next;
    }

    return 0;
}
