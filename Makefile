# Copyright © 2017-2020 Johnothan King. All rights reserved.
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
CFLAGS   := -O2 -ffast-math -fomit-frame-pointer -fpic -fno-plt -pipe
INCLUDE  := -I./include
CPPFLAGS := -D_FORTIFY_SOURCE=2
WFLAGS   := -Wall -Wextra -Wno-unused-result
LDFLAGS  := -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now
#RCSHELL := /bin/dash

# Compile LeanInit
all: clean
	@mkdir -p out
	@cp -r rc out/rc
	@mv out/rc/svc/universal out/svc
	@mv out/rc/rc.conf.d out/rc.conf.d
	@cp rc/service out/rc/leaninit-service
	@rm -r out/rc/svc
	@if [ `uname` = FreeBSD ]; then \
		cp -r rc/svc/freebsd/* out/svc ;\
		rm -f out/rc.conf.d/cron.conf out/rc.conf.d/ntp.conf out/rc.conf.d/udev.conf ;\
		sed -i '' "/#DEF Linux/,/#ENDEF/d"   out/*/* ;\
		sed -i '' "/#DEF FreeBSD/,/#ENDEF/d" out/*/* ;\
		sed -i '' "/#DEF NetBSD/d" out/*/* ;\
		sed -i '' "/#ENDEF/d"      out/*/* ;\
		if [ "$(RCSHELL)" ]; then \
			sed -i '' "s:#!/bin/sh:#!$(RCSHELL):g" out/rc/* out/svc/* ;\
		fi ;\
		sed -i '' "s/    /	/g" out/*/* ;\
		$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) $(INCLUDE) -D`uname` -o out/os-indications cmd/os-indications.c $(LDFLAGS) -lefivar -lgeom ;\
	elif [ `uname` = NetBSD ]; then \
		cp -r rc/svc/netbsd/* out/svc ;\
		rm -f out/rc.conf.d/cron.conf out/rc.conf.d/udev.conf ;\
		sed -i "/#DEF Linux/,/#ENDEF/d"  out/*/* ;\
		sed -i "/#DEF NetBSD/,/#ENDEF/d" out/*/* ;\
		sed -i "/#DEF FreeBSD/d" out/*/* ;\
		sed -i "/#ENDEF/d"       out/*/* ;\
		if [ "$(RCSHELL)" ]; then \
			sed -i '' "s:#!/bin/sh:#!$(RCSHELL):g" out/rc/* out/svc/* ;\
		fi ;\
		sed -i "s/    /	/g" out/*/* ;\
	elif [ `uname` = Linux ]; then \
		cp -r rc/svc/linux/* out/svc ;\
		rm -f out/rc.conf.d/ntp.conf ;\
		sed -i "/#DEF FreeBSD/,/#ENDEF/d" out/*/* ;\
		sed -i "/#DEF NetBSD/,/#ENDEF/d"  out/*/* ;\
		sed -i "/#DEF Linux/d" out/*/* ;\
		sed -i "/#ENDEF/d"     out/*/* ;\
		if [ "$(RCSHELL)" ]; then \
			sed -i "s:#!/bin/sh:#!$(RCSHELL):g" out/rc/* out/svc/* ;\
		fi ;\
		sed -i "s/    /	/g" out/*/* ;\
		$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) $(INCLUDE) -D`uname` -o out/os-indications cmd/os-indications.c $(LDFLAGS) ;\
	else \
		echo "`uname` is not supported by LeanInit!" ;\
		false ;\
	fi
	@$(CC) $(CFLAGS) -pthread $(CPPFLAGS) $(WFLAGS) $(INCLUDE) -D`uname` -o out/leaninit cmd/init.c $(LDFLAGS) -lutil
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) $(INCLUDE) -D`uname` -o out/leaninit-halt cmd/halt.c $(LDFLAGS)
	@strip --strip-unneeded -R .comment -R .gnu.version -R .GCC.command.line -R .note.gnu.gold-version out/leaninit out/leaninit-halt out/os-indications
	@cp -r man out
	@echo "Successfully built LeanInit!"

# Install LeanInit's man pages and license
install-universal:
	@if [ ! -d out ]; then echo 'Please build LeanInit before installing either the RC system or LeanInit itself!'; false; fi
	@mkdir -p "$(DESTDIR)/usr/share/licenses/leaninit"
	@cp -r out/man "$(DESTDIR)/usr/share"
	@install -Dm0644 LICENSE "$(DESTDIR)/usr/share/licenses/leaninit/MIT"

# Install LeanInit RC for use with other init systems (symlink /etc/leaninit/rc to /etc/rc for this to take effect)
install-rc: install-universal
	@mkdir -p "$(DESTDIR)/sbin" "$(DESTDIR)/etc/leaninit/rc.conf.d" "$(DESTDIR)/var/log/leaninit" \
		"$(DESTDIR)/var/lib/leaninit/types" "$(DESTDIR)/var/lib/leaninit/svc"
	@cp -r out/svc "$(DESTDIR)/etc/leaninit"
	@cp -i out/rc.conf.d/* "$(DESTDIR)/etc/leaninit/rc.conf.d" || true
	@cp -i out/rc/rc.conf out/rc/ttys "$(DESTDIR)/etc/leaninit" || true
	@install -Dm0755 out/rc/rc out/rc/rc.svc out/rc/rc.shutdown "$(DESTDIR)/etc/leaninit"
	@install -Dm0755 out/rc/leaninit-service "$(DESTDIR)/sbin"
	@if [ `uname` = FreeBSD ] && [ ! -f "$(DESTDIR)/var/lib/leaninit/install-flag" ]; then \
		touch "$(DESTDIR)/var/lib/leaninit/svc/settings" "$(DESTDIR)/var/lib/leaninit/svc/swap" "$(DESTDIR)/var/lib/leaninit/svc/sysctl" \
			"$(DESTDIR)/var/lib/leaninit/svc/devd" "$(DESTDIR)/var/lib/leaninit/svc/zfs" "$(DESTDIR)/var/lib/leaninit/svc/syslogd" \
			"$(DESTDIR)/var/lib/leaninit/svc/powerd" "$(DESTDIR)/var/lib/leaninit/svc/wpa_supplicant" ;\
	elif [ `uname` = NetBSD ] && [ ! -f "$(DESTDIR)/var/lib/leaninit/install-flag" ]; then \
		touch "$(DESTDIR)/var/lib/leaninit/svc/settings" "$(DESTDIR)/var/lib/leaninit/svc/swap" "$(DESTDIR)/var/lib/leaninit/svc/sysctl" \
			"$(DESTDIR)/var/lib/leaninit/svc/syslogd" "$(DESTDIR)/var/lib/leaninit/svc/powerd" ;\
	elif [ `uname` = Linux ] && [ ! -f "$(DESTDIR)/var/lib/leaninit/install-flag" ]; then \
		touch "$(DESTDIR)/var/lib/leaninit/svc/settings" "$(DESTDIR)/var/lib/leaninit/svc/mountpfs" "$(DESTDIR)/var/lib/leaninit/svc/netface" \
			"$(DESTDIR)/var/lib/leaninit/svc/swap" "$(DESTDIR)/var/lib/leaninit/svc/sysctl" "$(DESTDIR)/var/lib/leaninit/svc/udev" ;\
		echo "udev" > "$(DESTDIR)/var/lib/leaninit/types/udev.type" ;\
		install -Dm0755 out/rc/rc.banner "$(DESTDIR)/etc/leaninit" ;\
	fi
	@touch "$(DESTDIR)/var/lib/leaninit/install-flag"
	@echo "Successfully installed LeanInit's RC system!"

# Install only the base of LeanInit (init, halt and os-indications)
# cd is used with ln(1) for POSIX-compliant relative symlinks
install-base: install-universal
	@mkdir -p "$(DESTDIR)/sbin"
	@cd "$(DESTDIR)/usr/share/man/man8" ;\
	[ -f leaninit-poweroff.8 ] || ln -sf leaninit-halt.8 leaninit-poweroff.8 ;\
	[ -f leaninit-reboot.8 ] || ln -sf leaninit-halt.8 leaninit-reboot.8
	@install -Dm0755 out/leaninit out/leaninit-halt out/os-indications "$(DESTDIR)/sbin"
	@cd "$(DESTDIR)/sbin"; ln -sf leaninit-halt leaninit-poweroff
	@cd "$(DESTDIR)/sbin"; ln -sf leaninit-halt leaninit-reboot
	@echo "Successfully installed the base of LeanInit!"


# Install LeanInit (does not overwrite other init systems)
install: install-rc install-base

# Set LeanInit as the default init system
override: install
	@cd "$(DESTDIR)/sbin" ;\
	ln -sf "leaninit"         "init" ;\
	ln -sf "leaninit-halt"    "halt" ;\
	ln -sf "leaninit-halt"    "poweroff" ;\
	ln -sf "leaninit-halt"    "reboot" ;\
	ln -sf "leaninit-service" "service" ;\
	@cd "$(DESTDIR)/etc" ;\
	ln -sf "leaninit/rc"          "rc" ;\
	ln -sf "leaninit/rc.banner"   "rc.banner" ;\
	ln -sf "leaninit/rc.conf"     "rc.conf" ;\
	ln -sf "leaninit/rc.conf.d"   "rc.conf.d" ;\
	ln -sf "leaninit/rc.shutdown" "rc.shutdown" ;\
	ln -sf "leaninit/rc.svc"      "rc.svc" ;\
	ln -sf "leaninit/svc"         "init.d" ;\
	ln -sf "leaninit/svc"         "rc.d" ;\
	ln -sf "leaninit/ttys"        "ttys" ;\
	@cd "$(DESTDIR)/usr/share/man" ;\
	ln -sf "man5/leaninit-rc.conf.5"     "man5/rc.conf.5" ;\
	ln -sf "man5/leaninit-ttys.5"        "man5/ttys.5" ;\
	ln -sf "man8/leaninit-halt.8"        "man8/halt.8" ;\
	ln -sf "man8/leaninit-halt.8"        "man8/poweroff.8" ;\
	ln -sf "man8/leaninit-halt.8"        "man8/reboot.8" ;\
	ln -sf "man8/leaninit-rc.8"          "man8/rc.8" ;\
	ln -sf "man8/leaninit-rc.banner.8"   "man8/rc.banner.8" ;\
	ln -sf "man8/leaninit-rc.shutdown.8" "man8/rc.shutdown.8" ;\
	ln -sf "man8/leaninit-rc.svc.8"      "man8/rc.svc.8" ;\
	ln -sf "man8/leaninit-service.8"     "man8/service.8" ;\
	ln -sf "man8/leaninit.8"             "man8/init.8" ;\
	echo "LeanInit is now the default init system!"

# Uninstall (only works with normal installations)
uninstall:
	@if [ `id -u` -ne 0 ]; then \
		echo "You must be root to uninstall LeanInit!" ;\
		false ;\
	fi
	@if [ ! -x "$(DESTDIR)/sbin/leaninit" ]; then \
		echo "Failed to detect an installation of LeanInit, exiting..." ;\
		false ;\
	fi
	@rm -rf "$(DESTDIR)/sbin/leaninit" "$(DESTDIR)/sbin/leaninit-halt" "$(DESTDIR)/sbin/leaninit-poweroff" "$(DESTDIR)/sbin/leaninit-reboot" "$(DESTDIR)/sbin/os-indications" \
		"$(DESTDIR)/sbin/leaninit-service" "$(DESTDIR)/etc/leaninit" "$(DESTDIR)/var/log/leaninit*" "$(DESTDIR)/var/run/leaninit"  "$(DESTDIR)/usr/share/licenses/leaninit" \
		"$(DESTDIR)/usr/share/man/man5/leaninit-rc.conf.5" "$(DESTDIR)/usr/share/man/man5/leaninit-ttys.5" "$(DESTDIR)/usr/share/man/man8/leaninit-rc.svc.8" \
		"$(DESTDIR)/usr/share/man/man8/leaninit.8" "$(DESTDIR)/usr/share/man/man8/leaninit-halt.8" "$(DESTDIR)/usr/share/man/man8/leaninit-rc.8" "$(DESTDIR)/usr/share/man/man8/leaninit-rc.banner.8" \
		"$(DESTDIR)/usr/share/man/man8/leaninit-rc.shutdown.8" "$(DESTDIR)/usr/share/man/man8/leaninit-service.8" "$(DESTDIR)/usr/share/man/man8/leaninit-poweroff.8" \
		"$(DESTDIR)/usr/share/man/man8/leaninit-reboot.8" "$(DESTDIR)/usr/share/man/man8/os-indications.8" "$(DESTDIR)/usr/share/man/man8/leaninit-poweroff.8" \
		"$(DESTDIR)/usr/share/man/man8/leaninit-reboot.8" "$(DESTDIR)/var/lib/leaninit"
	@echo "Successfully uninstalled LeanInit!"
	@echo "Please make sure you remove LeanInit from your bootloader!"

# Clean the directory
clean:
	@rm -rf out
	@git gc >> /dev/null 2> /dev/null
	@git repack >> /dev/null 2> /dev/null

# Calls clean, then resets the git repo
clobber: clean
	git reset --hard

# Alias for clobber
mrproper: clobber
