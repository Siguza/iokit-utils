ALL     = ioclass ioprint ioscan
INCDIR  = include
CFLAGS  ?= -Wall -O3 -framework IOKit -framework CoreFoundation
LIBKERN ?= /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include
IOKIT   ?= /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/IOKit.framework/Headers

.PHONY: all clean

all: $(ALL)

%: src/%.c | $(INCDIR)
	$(CC) -o $@.osx $< $(CFLAGS)
	xcrun -sdk iphoneos $(CC) -arch armv7 -arch arm64 -o $@.ios $< $(CFLAGS) -I$(INCDIR)
	lipo -create -output $@ $@.osx $@.ios
	codesign -s - $@
	rm -f $@.osx $@.ios

$(INCDIR):
	mkdir $(INCDIR)
	ln -s $(IOKIT) $(INCDIR)/IOKit
	mkdir $(INCDIR)/libkern
	ln -s $(LIBKERN)/libkern/OSTypes.h $(INCDIR)/libkern/OSTypes.h

clean:
	rm -f $(ALL)
