VERSION     = 1.3.1
BINDIR      = bin
SRCDIR      = src
ALL         = $(patsubst $(SRCDIR)/%.c,%,$(wildcard $(SRCDIR)/io*.c))
PKG         = pkg
XZ          = iokit-utils.tar.xz
DEB         = net.siguza.iokit-utils_$(VERSION)_iphoneos-arm.deb
C_FLAGS    ?= -Wall -O3 -framework IOKit -framework CoreFoundation $(CFLAGS)
CC_FLAGS   ?= -arch x86_64 -arch arm64
IOS_CC     ?= xcrun -sdk iphoneos clang
IOS_CFLAGS ?= -arch armv7 -arch arm64


.PHONY: all dist xz deb clean

all: $(addprefix $(BINDIR)/macos/, $(ALL)) $(addprefix $(BINDIR)/ios/, $(ALL))

$(BINDIR)/macos/%: $(SRCDIR)/%.c $(SRCDIR)/cfj.c | $(BINDIR)/macos
	$(CC) $(CC_FLAGS) $(C_FLAGS) -o $@ $< $(SRCDIR)/cfj.c
	codesign -s - $@

$(BINDIR)/ios/%: $(SRCDIR)/%.c $(SRCDIR)/cfj.c | $(BINDIR)/ios
	$(IOS_CC) $(IOS_CFLAGS) $(C_FLAGS) -o $@ $< $(SRCDIR)/cfj.c
	codesign -s - $@

dist: xz deb

xz: $(XZ)

deb: $(DEB)

$(XZ): $(addprefix $(BINDIR)/macos/, $(ALL)) $(addprefix $(BINDIR)/ios/, $(ALL))
	tar -cJf $(XZ) -C $(BINDIR) macos ios

$(DEB): $(PKG)/control.tar.gz $(PKG)/data.tar.lzma $(PKG)/debian-binary
	( cd "$(PKG)"; ar -cr "../$(DEB)" 'debian-binary' 'control.tar.gz' 'data.tar.lzma'; )

$(PKG)/control.tar.gz: $(PKG)/control
	tar -czf '$(PKG)/control.tar.gz' --exclude '.DS_Store' --exclude '._*' --exclude 'control.tar.gz' --include '$(PKG)' --include '$(PKG)/control' -s '%^$(PKG)%.%' $(PKG)

$(PKG)/data.tar.lzma: $(addprefix $(BINDIR)/ios/, $(ALL)) | $(PKG)
	tar -c --lzma -f '$(PKG)/data.tar.lzma' --exclude '.DS_Store' --exclude '._*' -s '%^bin/ios%./usr/bin%' @misc/template.tar $(BINDIR)/ios

$(PKG)/debian-binary: | $(PKG)
	echo '2.0' > "$(PKG)/debian-binary"

$(PKG)/control: misc/control | $(PKG)
	( echo "Version: $(VERSION)"; cat misc/control; ) > $(PKG)/control

$(BINDIR) $(BINDIR)/macos $(BINDIR)/ios $(PKG):
	mkdir -p $@

clean:
	rm -rf $(BINDIR) $(PKG) $(XZ) net.siguza.iokit-utils_*_iphoneos-arm.deb
