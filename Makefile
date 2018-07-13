# Copyright (c) 2017-2018 Johnothan King. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

# Variables (each one can be overridden)
CC      := cc
SED     := sed -i
WFLAGS  := -Wall -Wextra -Wpedantic
CFLAGS  := -O2 -fno-math-errno -pipe
OSFLAGS := -DLINUX
FORK    := setsid
KBD     := loadkeys
GETTY   := /sbin/agetty

# FreeBSD Compatibility
ifeq ($(shell uname),FreeBSD)
	OSFLAGS=-DFREEBSD
	SED=sed -i ''
	FORK=daemon
	KBD=setxkbmap
	GETTY=/usr/libexec/getty
endif

# Make the LeanInit binary
all:
	$(CC) $(WFLAGS) $(CFLAGS) $(OSFLAGS) -o l-init init.c $(LDFLAGS)
	$(CC) $(WFLAGS) $(CFLAGS) $(OSFLAGS) -o lsvc lsvc.c $(LDFLAGS)

# Install LeanInit (compatible with other init systems)
install: all
	mkdir -p $(DESTDIR)/sbin $(DESTDIR)/etc/leaninit/svce
	install -Dm0755 l-init lsvc $(DESTDIR)/sbin
	install -Dm0755 rc $(DESTDIR)/etc/leaninit
	cp -r svc xdm.conf $(DESTDIR)/etc/leaninit
	install -Dm0644 ttys $(DESTDIR)/etc/leaninit
	cd $(DESTDIR)/sbin && ln -sf l-init l-halt
	cd $(DESTDIR)/sbin && ln -sf l-init l-poweroff
	cd $(DESTDIR)/sbin && ln -sf l-init l-reboot
	$(SED) "s:FORK_PROG:$(FORK):g" $(DESTDIR)/etc/leaninit/rc
	$(SED) "s:KBD_PROG:$(KBD):g" $(DESTDIR)/etc/leaninit/rc
	$(SED) "s:GETTY_PROG:$(GETTY):g" $(DESTDIR)/etc/leaninit/ttys

# Compile LeanInit without regard for other init systems
override:
	$(CC) $(WFLAGS) $(CFLAGS) $(OSFLAGS) -DOVERRIDE -o init init.c $(LDFLAGS)
	$(CC) $(WFLAGS) $(CFLAGS) $(OSFLAGS) -DOVERRIDE -o lsvc lsvc.c $(LDFLAGS)

# Install LeanInit without regard for other init systems
override_install: override
	mkdir -p $(DESTDIR)/sbin $(DESTDIR)/etc/leaninit/svce
	install -Dm0755 init lsvc $(DESTDIR)/sbin
	install -Dm0755 rc $(DESTDIR)/etc
	cp -r svc xdm.conf $(DESTDIR)/etc/leaninit
	install -Dm0644 ttys $(DESTDIR)/etc/leaninit
	cd $(DESTDIR)/sbin && ln -sf init halt
	cd $(DESTDIR)/sbin && ln -sf init poweroff
	cd $(DESTDIR)/sbin && ln -sf init reboot
	$(SED) "s:FORK_PROG:$(FORK):g" $(DESTDIR)/etc/rc
	$(SED) "s:KBD_PROG:$(KBD):g" $(DESTDIR)/etc/rc
	$(SED) "s:GETTY_PROG:$(GETTY):g" $(DESTDIR)/etc/leaninit/ttys

# Clean the directory
clean:
	rm -f init l-init lsvc

# Calls clean, then resets the git repo
clobber: clean
	git reset --hard

# Alias for clobber
mrproper: clobber
