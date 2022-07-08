# IOKit Utils

Just some little dev tools to probe IOKit.  
Makefile is designed to build all-in-one binaries for both iOS and macOS.

# `ioclass`

Usage:

    ioclass [-b] [Name]

Takes an IOKit class name as argument and, if `-b` is given, prints the bundle ID of the providing kext, otherwise prints its class hierarchy.

### Example

    bash$ ioclass RootDomainUserClient
    RootDomainUserClient
     IOUserClient
      IOService
       IORegistryEntry
        OSObject

# `ioprint`

Iterate over all entries in a registry plane and perform operations on them.

Usage:

    ioprint [-d] [-h] [-p Plane] [-s] [Name]

All arguments are optional.  
Class names of all considered objects as well as return values are always printed.

- `Name`: Limit the performed operations to only objects that either extend a class `Name`, or whose name in the registry is `Name`. If none is given, all objects are processed.
- `-d`: Print the registry properties of all objects.
- `-h`: Print a help and exit.
- `-s`: Try to set properties `<key>herp</key><string>derp</string>` on all objects.
- `-p Plane`: Iterate over registry plane `Plane`. Default is `IOService`.

If both are given, `-s` takes precedence over `-d`.

### Examples

List all entries of the `IOService` plane:

    bash$ ioprint
    # [ excessive output omitted ]

List all entries of the `IOUSB` plane:

    bash$ ioprint -p IOUSB
    IORegistryEntry(Root)
    IOUSBRootHubDevice(Root Hub Simulation Simulation)
    IOUSBDevice(Bluetooth USB Host Controller)

List all `IOUserClient` instances:

    bash$ ioprint IOUserClient
    # [ excessive output omitted ]

Print properties of all `IOHIDUserClient` instances:

    bash$ ioprint -d IOHIDUserClient
    IOHIDUserClient: (os/kern) successful (0x0)
    <?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
    <plist version="1.0">
    <dict>
        <key>IOUserClientCreator</key>
        <string>pid 223, WindowServer</string>
        <key>IOUserClientCrossEndianCompatible</key>
        <true/>
    </dict>
    </plist>

List all properties of the registry root:

    bash$ ioprint -d Root
    # [ excessive output omitted ]

Try to set properties on all `IOHIDUserClient` instances:

    bash$ ioprint -s IOHIDUserClient
    IOHIDUserClient: (os/kern) successful (0x0)

(Note: The return value doesn't necessarily indicate that properties were actually set. Usually for user clients, they are not.)

# `ioscan`

Iterate over all entries in a registry plane and try to spawn user clients.  
Prints name and class of all services, whether spawning a client was successful, class of the spawned client, and whether multiple user clients have the same ID (i.e. are shared clients).

Usage:

    ioscan [-h] [-p Plane] [-s] [Name [min [max]]]

- `Name`: Limit the performed operations to only objects that either extend a class `Name`, or whose name in the registry is `Name`. If none is given, all objects are processed.
- `min` and `max`: Try spawning user clients of certain types (can be given in base 8, 10 or 16). If both `min` and `max` are given, all types in that range will be tried. If only `min` is given, that one type will be tried. Defaults to `0`.
- `-h`: Print a help and exit.
- `-s`: Only print entries where a user client was successfully spawned.
- `-p Plane`: Iterate over registry plane `Plane`. Default is `IOService`.

All arguments are optional, but `min` and `max` can only be given if `Name` is given too.

### Examples

Spawn a user client for every service:

    bash$ ioscan
    # [ excessive output omitted ]

Spawn an AMFI user client:

    bash$ ioscan AppleMobileFileIntegrity
    Class                    Name                     Type Spawn                UC                                   One   Two Equal
    AppleMobileFileIntegrity AppleMobileFileIntegrity    0 (os/kern) successful AppleMobileFileIntegrityUserClient 23207 23107 !=   

Spawn an `IOGraphicsDevice` user client of type `1`:

    bash$ ioscan IOGraphicsDevice 1
    Class                 Name                  Type Spawn                UC                             One  Two Equal
    AppleIntelFramebuffer AppleIntelFramebuffer    1 (os/kern) successful IOFramebufferSharedUserClient 8307 8307 ==   
    AppleIntelFramebuffer AppleIntelFramebuffer    1 (os/kern) successful IOFramebufferSharedUserClient 8d07 8d07 ==   
    AppleIntelFramebuffer AppleIntelFramebuffer    1 (os/kern) successful IOFramebufferSharedUserClient 9407 9407 ==   

### License

[MPL2](https://github.com/Siguza/iokit-utils/blob/master/LICENSE) with Exhibit B, except for [`iokit.h`](https://github.com/Siguza/iokit-utils/blob/master/src/iokit.h) which is Public Domain.
