VERSION = 1.3.0
BINDIR  = bin
SRCDIR  = src
ALL     = $(patsubst $(SRCDIR)/%.c,%,$(wildcard $(SRCDIR)/io*.c))
PKG     = pkg
XZ      = iokit-utils.tar.xz
DEB     = net.siguza.iokit-utils_$(VERSION)_iphoneos-arm.deb
CFLAGS  ?= -Wall -O3 -framework IOKit -framework CoreFoundation
LIBKERN ?= /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include
IOKIT   ?= /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/IOKit.framework/Headers

.PHONY: all dist xz deb clean

all: $(addprefix $(BINDIR)/, $(ALL))

$(BINDIR)/%: $(SRCDIR)/%.c | $(BINDIR)
	$(CC) -o $@.osx $< $(SRCDIR)/cfj.c $(CFLAGS)
	xcrun -sdk iphoneos $(CC) -arch armv7 -arch arm64 -o $@.ios $< $(SRCDIR)/cfj.c $(CFLAGS)
	lipo -create -output $@ $@.osx $@.ios
	codesign -s - $@
	rm -f $@.osx $@.ios

$(BINDIR):
	mkdir -p $(BINDIR)

dist: xz deb

xz: $(XZ)

deb: $(DEB)

$(XZ): $(addprefix $(BINDIR)/, $(ALL))
	tar -cJf $(XZ) -C $(BINDIR) $(ALL)

$(DEB): $(PKG)/control.tar.gz $(PKG)/data.tar.lzma $(PKG)/debian-binary
	( cd "$(PKG)"; ar -cr "../$(DEB)" 'debian-binary' 'control.tar.gz' 'data.tar.lzma'; )

$(PKG)/control.tar.gz: $(PKG)/control
	tar -czf '$(PKG)/control.tar.gz' --exclude '.DS_Store' --exclude '._*' --exclude 'control.tar.gz' --include '$(PKG)' --include '$(PKG)/control' -s '%^$(PKG)%.%' $(PKG)

$(PKG)/data.tar.lzma: $(addprefix $(BINDIR)/, $(ALL)) | $(PKG)
	tar -c --lzma -f '$(PKG)/data.tar.lzma' --exclude '.DS_Store' --exclude '._*' -s '%^bin%./usr/bin%' @misc/template.tar $(BINDIR)

$(PKG)/debian-binary: $(addprefix $(BINDIR)/, $(ALL)) | $(PKG)
	echo '2.0' > "$(PKG)/debian-binary"

$(PKG)/control: misc/control | $(PKG)
	( echo "Version: $(VERSION)"; cat misc/control; ) > $(PKG)/control

$(PKG):
	mkdir -p $(PKG)

clean:
	rm -rf $(BINDIR) $(PKG) $(XZ) net.siguza.iokit-utils_*_iphoneos-arm.deb
