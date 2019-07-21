# Copyright (c) 2017-2019 Johnothan King. All rights reserved.
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
CC        := cc
CFLAGS    := -O2 -ffast-math -fomit-frame-pointer -fPIC -pipe -I./include
#CPPFLAGS := -DUINT32
WFLAGS    := -Wall -Wextra
LDFLAGS   := -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now
INSTALL   := install
SED       := sed
STRIP     := strip
XZ        := xz
MANPAGES  := $(DESTDIR)/usr/share/man/man5/leaninit-rc.conf.5.xz $(DESTDIR)/usr/share/man/man5/leaninit-ttys.5.xz $(DESTDIR)/usr/share/man/man8/leaninit-rc.svc.8.xz \
	$(DESTDIR)/usr/share/man/man8/leaninit.8.xz $(DESTDIR)/usr/share/man/man8/leaninit-halt.8.xz $(DESTDIR)/usr/share/man/man8/leaninit-rc.8.xz \
	$(DESTDIR)/usr/share/man/man8/leaninit-rc.shutdown.8.xz $(DESTDIR)/usr/share/man/man8/leaninit-service.8.xz $(DESTDIR)/usr/share/man/man8/leaninit-poweroff.8.xz \
	$(DESTDIR)/usr/share/man/man8/leaninit-reboot.8.xz $(DESTDIR)/usr/share/man/man8/leaninit-zzz.8.xz $(DESTDIR)/usr/share/man/man8/os-indications.8.xz

# Compile LeanInit
all: clean
	@mkdir -p out
	@cp -r rc out/rc
	@mv out/rc/svc/universal out/svc
	@mv out/rc/rc.conf.d out/rc.conf.d
	@cp rc/service.sh out/rc/leaninit-service
	@if [ `uname` = FreeBSD ]; then \
		cp -r rc/svc/freebsd/* out/svc && rm -r out/rc/svc ;\
		$(SED) -i '' "/#DEF Linux/,/#ENDEF/d" out/*/* ;\
		$(SED) -i '' "/#DEF FreeBSD/d"        out/*/* ;\
		$(SED) -i '' "/#ENDEF/d"              out/*/* ;\
		$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) -D`uname` -o out/os-indications cmd/os-indications.c $(LDFLAGS) -lefivar ;\
	elif [ `uname` = Linux ]; then \
		cp -r rc/svc/linux/* out/svc && rm -r out/rc/svc ;\
		$(SED) -i "/#DEF FreeBSD/,/#ENDEF/d" out/*/* ;\
		$(SED) -i "/#DEF Linux/d"            out/*/* ;\
		$(SED) -i "/#ENDEF/d"                out/*/* ;\
		$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) -D`uname` -o out/os-indications cmd/os-indications.c $(LDFLAGS) ;\
	else \
		echo "`uname` is not supported by LeanInit!" ;\
		false ;\
	fi
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) -D`uname` -o out/leaninit      cmd/init.c $(LDFLAGS) -lpthread -lutil
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) -D`uname` -o out/leaninit-halt cmd/halt.c $(LDFLAGS)
	@$(STRIP) --strip-unneeded -R .comment -R .gnu.version -R .GCC.command.line -R .note.gnu.gold-version out/leaninit out/leaninit-halt out/os-indications
	@cp -r man out
	@$(XZ) -T 0 out/man/*/*
	@echo "Successfully built LeanInit!"

# Install LeanInit's rc system for use with other init systems (symlink /etc/leaninit/rc to /etc/rc for this to take effect)
install-rc:
	@if [ ! -d out ]; then echo 'Please build LeanInit before installing either the RC system or LeanInit itself'; false; fi
	@mkdir -p  $(DESTDIR)/sbin $(DESTDIR)/etc/leaninit/rc.conf.d $(DESTDIR)/usr/share/licenses/leaninit $(DESTDIR)/var/log \
		$(DESTDIR)/var/run/leaninit $(DESTDIR)/var/lib/leaninit/types $(DESTDIR)/var/lib/leaninit/svc
	@cp -r out/svc    $(DESTDIR)/etc/leaninit
	@cp -r out/man    $(DESTDIR)/usr/share
	@cp -i out/rc.conf.d/* $(DESTDIR)/etc/leaninit/rc.conf.d || true
	@cp -i out/rc/rc.conf out/rc/ttys $(DESTDIR)/etc/leaninit || true
	@$(INSTALL) -Dm0644 LICENSE $(DESTDIR)/usr/share/licenses/leaninit/MIT
	@$(INSTALL) -Dm0755 out/rc/rc out/rc/rc.svc out/rc/rc.shutdown $(DESTDIR)/etc/leaninit
	@$(INSTALL) -Dm0755 out/rc/leaninit-service $(DESTDIR)/sbin
	@if [ `uname` = FreeBSD ]; then \
		touch $(DESTDIR)/var/lib/leaninit/svc/settings $(DESTDIR)/var/lib/leaninit/svc/swap $(DESTDIR)/var/lib/leaninit/svc/sysctl \
			$(DESTDIR)/var/lib/leaninit/svc/devd $(DESTDIR)/var/lib/leaninit/svc/zfs ;\
	elif [ `uname` = Linux ]; then \
		touch $(DESTDIR)/var/lib/leaninit/svc/kmod $(DESTDIR)/var/lib/leaninit/svc/linux-settings $(DESTDIR)/var/lib/leaninit/svc/mountpfs \
			$(DESTDIR)/var/lib/leaninit/svc/settings $(DESTDIR)/var/lib/leaninit/svc/swap $(DESTDIR)/var/lib/leaninit/svc/sysctl \
			$(DESTDIR)/var/lib/leaninit/svc/udev ;\
		echo "udev" > $(DESTDIR)/var/lib/leaninit/types/udev.type ;\
	fi
	@echo "Successfully installed LeanInit's RC system!"

# Install LeanInit (does not overwrite other init systems)
install: install-rc
	@cd $(DESTDIR)/usr/share/man/man8 ;\
	[ -r leaninit-poweroff.8 ] || ln -sf leaninit-halt.8 leaninit-poweroff.8 ;\
	[ -r leaninit-reboot.8 ] || ln -sf leaninit-halt.8 leaninit-reboot.8 ;\
	if [ `uname` = Linux ]; then [ -r leaninit-zzz.8 ] || ln -sf leaninit-halt.8 leaninit-zzz.8; fi
	@$(INSTALL) -Dm0755 out/leaninit out/leaninit-halt out/os-indications $(DESTDIR)/sbin
	@cd $(DESTDIR)/sbin; ln -sf leaninit-halt leaninit-poweroff
	@cd $(DESTDIR)/sbin; ln -sf leaninit-halt leaninit-reboot
	@if [ `uname` = Linux ]; then cd $(DESTDIR)/sbin; ln -sf leaninit-halt leaninit-zzz; fi
	@echo "Successfully installed LeanInit itself!"

# Uninstall (only works with normal installations)
uninstall:
	@if [ `id -u` != 0 ]; then \
		echo "You must be root to uninstall LeanInit!" ;\
		false ;\
	fi
	@if [ ! -x $(DESTDIR)/sbin/leaninit ]; then \
		echo "Failed to detect an installation of LeanInit, exiting..." ;\
		false ;\
	fi
	@rm -rf $(DESTDIR)/sbin/leaninit $(DESTDIR)/sbin/leaninit-halt $(DESTDIR)/sbin/leaninit-poweroff $(DESTDIR)/sbin/leaninit-reboot $(DESTDIR)/sbin/os-indications $(DESTDIR)/usr/share/licenses/leaninit \
		$(DESTDIR)/sbin/leaninit-zzz $(DESTDIR)/sbin/leaninit-service $(DESTDIR)/etc/leaninit $(DESTDIR)/var/log/leaninit.log $(DESTDIR)/var/run/leaninit $(MANPAGES)
	@echo "Successfully uninstalled LeanInit!"
	@echo "Please make sure you remove LeanInit from your bootloader!"

# Clean the directory
clean:
	@rm -rf out

# Calls clean, then resets the git repo
clobber: clean
	git reset --hard

# Alias for clobber
mrproper: clobber
