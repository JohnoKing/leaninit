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

# Variables (each one may be overridden)
CC       := cc
SED      := sed
INSTALL  := install
CFLAGS   := -O2 -ffast-math -fomit-frame-pointer -fPIC -pipe
WFLAGS   := -Wall -Wextra -Wpedantic -Wno-unused-result
LIBS     := -lpthread -lutil
RC       := out/*/*
MANPAGES := $(DESTDIR)/usr/share/man/man5/lrc.conf.5 $(DESTDIR)/usr/share/man/man5/lttys.5 $(DESTDIR)/usr/share/man/man8/rc.svc.8 \
	$(DESTDIR)/usr/share/man/man8/leaninit.8 $(DESTDIR)/usr/share/man/man8/lhalt.8 $(DESTDIR)/usr/share/man/man8/lrc.8 $(DESTDIR)/usr/share/man/man8/lrunlevel.8 \
	$(DESTDIR)/usr/share/man/man8/lgetty.8 $(DESTDIR)/usr/share/man/man8/lrc.shutdown.8 $(DESTDIR)/usr/share/man/man8/lservice.8 $(DESTDIR)/usr/share/man/man8/lpoweroff.8 \
	$(DESTDIR)/usr/share/man/man8/lreboot.8 $(DESTDIR)/usr/share/man/man8/lzzz.8

# Compile LeanInit
all: clean
	mkdir -p  out
	cp -r rc  out/rc
	cp -r svc out/svc.d
	cp cmd/service.sh  out/rc/lservice
	cp cmd/runlevel.sh out/rc/lrunlevel
	if [ `uname` = Linux ]; then \
		$(SED) -i "/DEFBSD/,/ENDEF/d" $(RC) ;\
		$(SED) -i "/DEFLINUX/d"       $(RC) ;\
		$(SED) -i "/ENDEF/d"          $(RC) ;\
		$(SED) -i "s:TTY:tty:g" out/rc/ttys ;\
	elif [ `uname` = FreeBSD ]; then \
		$(SED) -i '' "/DEFLINUX/,/ENDEF/d" $(RC) ;\
		$(SED) -i '' "/DEFBSD/d"           $(RC) ;\
		$(SED) -i '' "/ENDEF/d"            $(RC) ;\
		$(SED) -i '' "s:TTY:ttyv:g"  out/rc/ttys ;\
	else \
		echo "`uname` is not supported by LeanInit!" ;\
		false ;\
	fi
	$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/leaninit cmd/init.c   $(LDFLAGS) $(LIBS)
	$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/lhalt    cmd/halt.c   $(LDFLAGS) $(LIBS)
	$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/lgetty   cmd/lgetty.c $(LDFLAGS) $(LIBS)

# Install LeanInit (compatible with other init systems)
install:
	if [ ! -d out ]; then echo 'Please build LeanInit before attempting `make install`'; false; fi
	mkdir -p  $(DESTDIR)/sbin $(DESTDIR)/etc/leaninit/svc.e $(DESTDIR)/usr/share/licenses/leaninit $(DESTDIR)/var/log $(DESTDIR)/var/run/leaninit
	cp -r out/svc.d $(DESTDIR)/etc/leaninit
	cp -r man $(DESTDIR)/usr/share
	$(INSTALL) -Dm0644 LICENSE $(DESTDIR)/usr/share/licenses/leaninit/MIT
	if [ ! -r $(DESTDIR)/etc/leaninit/rc.conf ]; then \
		$(INSTALL) -Dm0644 out/rc/rc.conf $(DESTDIR)/etc/leaninit ;\
	fi
	if [ ! -r $(DESTDIR)/etc/leaninit/ttys ]; then \
		$(INSTALL) -Dm0644 out/rc/ttys $(DESTDIR)/etc/leaninit ;\
	fi
	$(INSTALL) -Dm0755 out/rc/rc out/rc/rc.svc out/rc/rc.shutdown $(DESTDIR)/etc/leaninit
	cd $(DESTDIR)/usr/share/man/man8 ;\
	[ -r lpoweroff.8 ] || ln -sf lhalt.8 lpoweroff.8 ;\
	[ -r lreboot.8 ] || ln -sf lhalt.8 lreboot.8 ;\
	if [ `uname` = Linux ]; then [ -r lzzz.8 ] || ln -sf lhalt.8 lzzz.8; fi
	$(INSTALL) -Dm0755 out/leaninit out/lhalt out/rc/lservice out/rc/lrunlevel out/lgetty $(DESTDIR)/sbin
	cd $(DESTDIR)/sbin; ln -sf lhalt lpoweroff
	cd $(DESTDIR)/sbin; ln -sf lhalt lreboot
	if [ `uname` = Linux ]; then cd $(DESTDIR)/sbin; ln -sf lhalt lzzz; fi
	if [ ! -r $(DESTDIR)/etc/leaninit/svc.e/getty.type ]; then touch $(DESTDIR)/etc/leaninit/svc.e/lgetty && echo lgetty > $(DESTDIR)/etc/leaninit/svc.e/getty.type; fi

# Uninstall (only works with normal installations)
uninstall:
	if [ `id -u` != 0 ]; then \
		echo "You must be root to uninstall LeanInit!" ;\
		false ;\
	fi
	if [ ! -x $(DESTDIR)/sbin/leaninit ]; then \
		echo "Failed to detect an installation of LeanInit, exiting..." ;\
		false ;\
	fi
	echo "Please make sure you remove LeanInit from your bootloader after uninstalling!"
	rm -rf $(DESTDIR)/sbin/leaninit $(DESTDIR)/sbin/lhalt $(DESTDIR)/sbin/lpoweroff $(DESTDIR)/sbin/lreboot $(DESTDIR)/sbin/lgetty $(DESTDIR)/usr/share/licenses/leaninit \
		$(DESTDIR)/sbin/lzzz $(DESTDIR)/sbin/lservice $(DESTDIR)/sbin/lrunlevel $(DESTDIR)/etc/leaninit $(DESTDIR)/var/log/leaninit.log $(DESTDIR)/var/run/leaninit $(MANPAGES)

# Clean the directory
clean:
	rm -rf out

# Calls clean, then resets the git repo
clobber: clean
	git reset --hard

# Alias for clobber
mrproper: clobber
