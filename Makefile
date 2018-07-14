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
SHDEF   := DEFBSD
NSHDEF  := DEFLINUX
TTY     := tty

# FreeBSD Compatibility
ifeq ($(shell uname),FreeBSD)
	OSFLAGS=-DFREEBSD
	SED=sed -i ''
	FORK=daemon
	KBD=setxkbmap
	GETTY=/usr/libexec/getty
	SHDEF=DEFLINUX
	NSHDEF=DEFBSD
	TTY=ttyv
endif

# Make the LeanInit binary
all:
	$(CC) $(WFLAGS) $(CFLAGS) $(OSFLAGS) -o linit init.c $(LDFLAGS)
	$(CC) $(WFLAGS) $(CFLAGS) $(OSFLAGS) -o lsvc lsvc.c $(LDFLAGS)
	cp rc.sh rc
	cp ttys.cfg ttys
	$(SED) "/$(SHDEF)/,/ENDEF/d" rc
	$(SED) "s/$(NSHDEF)//g" rc
	$(SED) "s/ENDEF//g" rc
	$(SED) "s:FORK_PROG:$(FORK):g" rc
	$(SED) "s:KBD_PROG:$(KBD):g" rc
	$(SED) "s:GETTY_PROG:$(GETTY):g" ttys
	$(SED) "s:TTY:$(TTY):g" ttys

# Compile LeanInit without regard for other init systems
override:
	$(CC) $(WFLAGS) $(CFLAGS) $(OSFLAGS) -DOVERRIDE -o init init.c $(LDFLAGS)
	$(CC) $(WFLAGS) $(CFLAGS) $(OSFLAGS) -DOVERRIDE -o lsvc lsvc.c $(LDFLAGS)

# Used by both install and override-install
install-base:
	mkdir -p $(DESTDIR)/sbin $(DESTDIR)/etc/leaninit/svce $(DESTDIR)/usr/share/licenses/leaninit
	cp -r svc xdm.conf $(DESTDIR)/etc/leaninit
	install -Dm0644 LICENSE $(DESTDIR)/usr/share/licenses/leaninit/MIT
	install -Dm0644 ttys $(DESTDIR)/etc/leaninit

# Install LeanInit (compatible with other init systems)
install: all install-base
	install -Dm0755 linit lsvc $(DESTDIR)/sbin
	install -Dm0755 rc $(DESTDIR)/etc/leaninit
	cd $(DESTDIR)/sbin && ln -sf linit lhalt
	cd $(DESTDIR)/sbin && ln -sf linit lpoweroff
	cd $(DESTDIR)/sbin && ln -sf linit lreboot

# Install LeanInit without regard for other init systems
override-install: override install-base
	install -Dm0755 init lsvc $(DESTDIR)/sbin
	install -Dm0755 rc $(DESTDIR)/etc
	cd $(DESTDIR)/sbin && ln -sf init halt
	cd $(DESTDIR)/sbin && ln -sf init poweroff
	cd $(DESTDIR)/sbin && ln -sf init reboot

# Clean the directory
clean:
	rm -f init linit lsvc rc ttys

# Calls clean, then resets the git repo
clobber: clean
	git reset --hard

# Alias for clobber
mrproper: clobber
