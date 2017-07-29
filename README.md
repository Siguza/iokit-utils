# IOKit Utils

Just some little dev tools to probe IOKit.  
Makefile is designed to build all-in-one binaries for both iOS and macOS.

# `ioclass`

Takes an IOKit class name as argument and prints its class hierarchy.  

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

    ioprint [-s] [-d] [-p Plane] [Name]

All arguments are optional.  
Class names of all considered objects as well as return values are always printed.

- `Name`: Limit the performed operations to only objects that either extend a class `Name`, or whose name in the registry is `Name`. If none is given, all objects are processed.
- `-d`: Print the registry properties of all objects.
- `-s`: Try to set properties `<key>herp</key><string>derp</string>` on all objects.
- `-p Plane`: Iterate over registry plane `Plane`. Default is `IOService`.

If both are given, `-s` takes precedence over `-d`.

### Examples

List all entries of the `IOService` plane:

    bash$ ioprint
    # [ excessive output omitted ]

List all entries of the `IOUSB` plane:

    bash$ ioprint -p IOUSB
    IOUSBRootHubDevice
    IOUSBDevice
    IOUSBDevice
    IOUSBDevice
    IOUSBDevice
    IOUSBDevice
    IOUSBDevice
    IOUSBDevice

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

Iterate over `IOService`s and try to spawn user clients.  
Prints name and class of all services, whether spawning a client was successful, and whether multiple user clients have the same ID (i.e. are shared clients).

Usage:

    ioscan [ClassName [type]]

- `ClassName`: Only iterate over services extending `ClassName`. Default is `IOService`.
- `type`: Try spawning a user client of that type (can be given in base 8, 10 or 16). Default is `0`.

Both arguments are optional, but `type` can only be given if `ClassName` is given too.

### Examples

Spawn a user client for every service:

    bash$ ioscan
    # [ excessive output omitted ]

Spawn an AMFI user client:

    bash$ ioscan AppleMobileFileIntegrity
    Class                                   Name                                    Spawn                                                            one    two    equal
    AppleMobileFileIntegrity                AppleMobileFileIntegrity                (os/kern) successful                                             3843   4099   !=

Spawn an `IOGraphicsDevice` user client of type `1`:

    bash$ ioscan IOGraphicsDevice 1
    Class                                   Name                                    Spawn                                                            one    two    equal
    NVDA                                    NVDA                                    (os/kern) successful                                             3843   3843   ==
    NVDA                                    NVDA                                    (os/kern) successful                                             4099   4099   ==
    NVDA                                    NVDA                                    (os/kern) successful                                             4355   4355   ==
