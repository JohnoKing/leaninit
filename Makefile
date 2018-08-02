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
CC      := cc
SED     := sed
INSTALL := install
WFLAGS  := -Wall -Wextra -Wpedantic
CFLAGS  := -O2 -fno-math-errno -pipe
LIBS    := -lutil
RC      := rc rc.api rc.conf ttys

# Compile LeanInit
all: sh-all
	$(CC) $(WFLAGS) $(CFLAGS) -D`uname` -DCOMPAT -o out/linit cmd/init.c $(LDFLAGS) $(LIBS)
	$(CC) $(WFLAGS) $(CFLAGS) -D`uname` -DCOMPAT -o out/lhalt cmd/halt.c $(LDFLAGS) $(LIBS)
	$(CC) $(WFLAGS) $(CFLAGS) -D`uname` -DCOMPAT -o out/lsvc  cmd/lsvc.c $(LDFLAGS) $(LIBS)

# Compile LeanInit without regard for other init systems
override: sh-all
	$(CC) $(WFLAGS) $(CFLAGS) -D`uname` -o out/init  cmd/init.c $(LDFLAGS) $(LIBS)
	$(CC) $(WFLAGS) $(CFLAGS) -D`uname` -o out/halt  cmd/halt.c $(LDFLAGS) $(LIBS)
	$(CC) $(WFLAGS) $(CFLAGS) -D`uname` -o out/lsvc  cmd/lsvc.c $(LDFLAGS) $(LIBS)

# Run sed on the scripts and config files
sh-all:
	mkdir -p      out
	cp rc/rc      out/rc
	cp rc/rc.api  out/rc.api
	cp rc/ttys    out/ttys
	cp rc/rc.conf out/rc.conf
	cd out ;\
	if [ `uname` = Linux ]; then \
		$(SED) -i "/DEFBSD/,/ENDEF/d" $(RC) ;\
		$(SED) -i "/DEFLINUX/d"     $(RC) ;\
		$(SED) -i "/ENDEF/d"        $(RC) ;\
		$(SED) -i "s:GETTY_PROG:/sbin/agetty:g" ttys ;\
		$(SED) -i "s:TTY:tty:g"                 ttys ;\
	elif [ `uname` = FreeBSD ]; then \
		$(SED) -i '' "/DEFLINUX/,/ENDEF/d" $(RC) ;\
		$(SED) -i '' "/DEFBSD/d"         $(RC) ;\
		$(SED) -i '' "/ENDEF/d"          $(RC) ;\
		$(SED) -i '' "s:GETTY_PROG:/usr/libexec/getty:g" ttys ;\
		$(SED) -i '' "s:TTY:ttyv:g"                      ttys ;\
	else \
		echo "`uname` is not supported by LeanInit!" ;\
		false ;\
	fi
	$(CC) $(WFLAGS) $(CFLAGS) -D`uname` -o out/fork  cmd/fork.c $(LDFLAGS) $(LIBS)

# Used by both install and override-install
install-base:
	mkdir -p  $(DESTDIR)/usr/bin $(DESTDIR)/sbin $(DESTDIR)/etc/leaninit/svce $(DESTDIR)/usr/share/licenses/leaninit
	cp -r man $(DESTDIR)/usr/share
	cp -r svc $(DESTDIR)/etc/leaninit
	$(INSTALL) -Dm0644 LICENSE $(DESTDIR)/usr/share/licenses/leaninit/MIT
	if [ ! -r $(DESTDIR)/etc/leaninit/rc.conf ]; then \
		$(INSTALL) -Dm0644 out/rc.conf $(DESTDIR)/etc/leaninit ;\
	fi
	if [ ! -r $(DESTDIR)/etc/leaninit/ttys ]; then \
		$(INSTALL) -Dm0644 out/ttys $(DESTDIR)/etc/leaninit ;\
	fi
	$(INSTALL) -Dm0755 out/rc.api rc/svc-start rc/svc-stop $(DESTDIR)/etc/leaninit
	$(INSTALL) -Dm0755 out/fork $(DESTDIR)/usr/bin/fork

# Install LeanInit (compatible with other init systems)
install: all install-base
	$(INSTALL) -Dm0755 out/linit out/lhalt out/lsvc $(DESTDIR)/sbin
	$(INSTALL) -Dm0755 out/rc $(DESTDIR)/etc/leaninit
	cd $(DESTDIR)/sbin && ln -sf lhalt lpoweroff
	cd $(DESTDIR)/sbin && ln -sf lhalt lreboot
	if [ `uname` = Linux ]; then cd $(DESTDIR)/sbin && ln -sf lhalt lzzz; fi

# Install LeanInit without regard for other init systems
override-install: override install-base
	$(INSTALL) -Dm0755 out/init out/halt out/lsvc $(DESTDIR)/sbin
	$(INSTALL) -Dm0755 out/rc $(DESTDIR)/etc
	cd $(DESTDIR)/sbin && ln -sf halt poweroff
	cd $(DESTDIR)/sbin && ln -sf halt reboot
	if [ `uname` = Linux ]; then cd $(DESTDIR)/sbin && ln -sf halt zzz; fi

# Uninstall (only works with normal installations)
uninstall:
	if [ `id -u` != 0 ]; then \
		echo "You must be root to uninstall LeanInit!" ;\
		false ;\
	fi
	if [ ! -r $(DESTDIR)/sbin/linit ]; then \
		echo "Failed to detect a normal installation of LeanInit, exiting..." ;\
		false ;\
	fi
	echo "Please make sure you remove LeanInit from your bootloader after uninstalling!"
	rm -rf $(DESTDIR)/sbin/linit $(DESTDIR)/sbin/lhalt $(DESTDIR)/sbin/lpoweroff $(DESTDIR)/sbin/lreboot $(DESTDIR)/usr/share/licenses/leaninit \
		$(DESTDIR)/sbin/lzzz $(DESTDIR)/sbin/lsvc $(DESTDIR)/usr/bin/fork $(DESTDIR)/etc/leaninit $(DESTDIR)/var/log/leaninit.log*

# Clean the directory
clean:
	rm -rf out

# Calls clean, then resets the git repo
clobber: clean
	git reset --hard

# Alias for clobber
mrproper: clobber
