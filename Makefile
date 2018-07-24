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

# Source Files
LINIT   := cmd/halt.c cmd/init.c
LSVC    := cmd/lsvc.c
RC      := rc rc.api ttys zfs.cfg

# Compile LeanInit
all: sh-all
	if [ `uname` = Linux ]; then \
		$(CC) $(WFLAGS) $(CFLAGS) -DLINUX -o out/linit   $(LINIT) $(LDFLAGS) ;\
		$(CC) $(WFLAGS) $(CFLAGS) -DLINUX -o out/lsvc    $(LSVC)  $(LDFLAGS) ;\
	elif [ `uname` = FreeBSD ]; then \
		$(CC) $(WFLAGS) $(CFLAGS) -DFREEBSD -o out/linit $(LINIT) $(LDFLAGS) ;\
		$(CC) $(WFLAGS) $(CFLAGS) -DFREEBSD -o out/lsvc  $(LSVC)  $(LDFLAGS) ;\
	else \
		echo "`uname` is not supported by LeanInit!" ;\
		false ;\
	fi

# Compile LeanInit without regard for other init systems
override: sh-all
	if [ `uname` = Linux ]; then \
		$(CC) $(WFLAGS) $(CFLAGS) -DLINUX -DOVERRIDE -o out/init   $(LINIT) $(LDFLAGS) ;\
		$(CC) $(WFLAGS) $(CFLAGS) -DLINUX -DOVERRIDE -o out/lsvc   $(LSVC)  $(LDFLAGS) ;\
	elif [ `uname` = FreeBSD ]; then \
		$(CC) $(WFLAGS) $(CFLAGS) -DFREEBSD -DOVERRIDE -o out/init $(LINIT) $(LDFLAGS) ;\
		$(CC) $(WFLAGS) $(CFLAGS) -DFREEBSD -DOVERRIDE -o out/lsvc $(LSVC)  $(LDFLAGS) ;\
	else \
		echo "`uname` is not supported by LeanInit!" ;\
		false ;\
	fi

# Run sed on $RC
sh-all:
	mkdir -p out
	cp rc/rc.sh      out/rc
	cp rc/rc.api.sh  out/rc.api
	cp rc/ttys       out/ttys
	cp rc/zfs.cfg    out/zfs.cfg
	cd out ;\
	if [ `uname` = Linux ]; then \
		$(SED) "/DEFBSD/,/ENDEF/d" $(RC) ;\
		$(SED) "s/DEFLINUX//g"     $(RC) ;\
		$(SED) "s/ENDEF//g"        $(RC) ;\
		$(SED) "s:GETTY_PROG:/sbin/agetty:g" ttys ;\
		$(SED) "s:TTY:tty:g"                 ttys ;\
	elif [ `uname` = FreeBSD ]; then \
		$(SED) '' "/DEFLINUX/,/ENDEF/d" $(RC) ;\
		$(SED) '' "s/DEFBSD//g"         $(RC) ;\
		$(SED) '' "s/ENDEF//g"          $(RC) ;\
		$(SED) '' "s:GETTY_PROG:/usr/libexec/getty:g" ttys ;\
		$(SED) '' "s:TTY:ttyv:g"                      ttys ;\
	else \
		echo "`uname` is not supported by LeanInit!" ;\
		false ;\
	fi

# Used by both install and override-install
install-base:
	mkdir -p $(DESTDIR)/sbin $(DESTDIR)/etc/leaninit/svce $(DESTDIR)/usr/share/licenses/leaninit
	cp -r svc $(DESTDIR)/etc/leaninit
	install -Dm0644 LICENSE $(DESTDIR)/usr/share/licenses/leaninit/MIT
	install -Dm0644 out/ttys rc/xdm.cfg out/zfs.cfg $(DESTDIR)/etc/leaninit
	install -Dm0755 out/rc.api rc/svc-start rc/svc-stop $(DESTDIR)/etc/leaninit

# Install LeanInit (compatible with other init systems)
install: all install-base
	install -Dm0755 out/linit out/lsvc $(DESTDIR)/sbin
	install -Dm0755 out/rc $(DESTDIR)/etc/leaninit
	cd $(DESTDIR)/sbin && ln -sf linit lhalt
	cd $(DESTDIR)/sbin && ln -sf linit lpoweroff
	cd $(DESTDIR)/sbin && ln -sf linit lreboot
	if [ `uname` = Linux ]; then cd $(DESTDIR)/sbin && ln -sf linit lzzz; fi

# Install LeanInit without regard for other init systems
override-install: override install-base
	install -Dm0755 out/init out/lsvc $(DESTDIR)/sbin
	install -Dm0755 out/rc $(DESTDIR)/etc
	cd $(DESTDIR)/sbin && ln -sf init halt
	cd $(DESTDIR)/sbin && ln -sf init poweroff
	cd $(DESTDIR)/sbin && ln -sf init reboot
	if [ `uname` = Linux ]; then cd $(DESTDIR)/sbin && ln -sf init zzz; fi

# Clean the directory
clean:
	rm -rf out

# Calls clean, then resets the git repo
clobber: clean
	git reset --hard

# Alias for clobber
mrproper: clobber
