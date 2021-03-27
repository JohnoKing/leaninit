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

# Variables
CC       := cc
CFLAGS   := -Os -fomit-frame-pointer -fpic -fno-math-errno -fno-plt -pipe
INCLUDE  := -I./include
CPPFLAGS := -D_FORTIFY_SOURCE=2
WFLAGS   := -Wall -Wextra -Wno-unused-result
LDFLAGS  := -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now
OUT      := out/man/man*/* out/rc/* out/rc.conf.d/* out/svc/*
#RCSHELL := /bin/dash

# Compile LeanInit
all: clean
	@mkdir -p out
	@cp -r man out
	@cp -r rc out/rc
	@mv out/rc/svc/universal out/svc
	@mv out/rc/rc.conf.d out/rc.conf.d
	@cp rc/service out/rc/leaninit-service
	@rm -r out/rc/svc
	@
	@# The point of using a custom preprocessor for shell scripts is to increase performance by
	@# avoiding unnecessary if statements such as `if [ $(uname) = Linux ]`.
	@# This will also build os-indications(8) if the operating system supports the APIs it depends on.
	@if [ `uname` = FreeBSD ]; then \
		cp -r rc/svc/freebsd/* out/svc ;\
		rm -f out/rc.conf.d/cron.conf out/rc.conf.d/udev.conf ;\
		sed -i '' "/#DEF Linux/,/#ENDEF/d"  $(OUT) ;\
		sed -i '' "/#DEF NetBSD/,/#ENDEF/d" $(OUT) ;\
		sed -i '' "/#DEF FreeBSD/d" $(OUT) ;\
		sed -i '' "/#DEF BSD/d"     $(OUT) ;\
		sed -i '' "/#ENDEF/d"       $(OUT) ;\
		[ "$(RCSHELL)" ] && sed -i '' "s:#!/bin/sh:#!$(RCSHELL):g" out/rc/* out/svc/* ;\
		sed -i '' "s/    /	/g" $(OUT) ;\
		$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) $(INCLUDE) -o out/os-indications cmd/os-indications.c $(LDFLAGS) -lefivar -lgeom ;\
		strip --strip-unneeded -R .comment -R .gnu.version -R .GCC.command.line -R .note.gnu.gold-version out/os-indications ;\
	\
	elif [ `uname` = NetBSD ]; then \
		cp -r rc/svc/netbsd/* out/svc ;\
		rm -f out/rc.conf.d/cron.conf out/rc.conf.d/udev.conf out/rc.conf.d/xdm.conf ;\
		sed -i "/#DEF Linux/,/#ENDEF/d"   $(OUT) ;\
		sed -i "/#DEF FreeBSD/,/#ENDEF/d" $(OUT) ;\
		sed -i "/#DEF NetBSD/d" $(OUT) ;\
		sed -i "/#DEF BSD/d"    $(OUT) ;\
		sed -i "/#ENDEF/d"      $(OUT) ;\
		[ "$(RCSHELL)" ] && sed -i '' "s:#!/bin/sh:#!$(RCSHELL):g" out/rc/* out/svc/* ;\
		sed -i "s/    /	/g" $(OUT) ;\
	\
	elif [ `uname` = Linux ]; then \
		cp -r rc/svc/linux/* out/svc ;\
		rm -f out/rc.conf.d/ntpd.conf ;\
		sed -i "/#DEF FreeBSD/,/#ENDEF/d" $(OUT) ;\
		sed -i "/#DEF NetBSD/,/#ENDEF/d"  $(OUT) ;\
		sed -i "/#DEF BSD/,/#ENDEF/d"     $(OUT) ;\
		sed -i "/#DEF Linux/d" $(OUT) ;\
		sed -i "/#ENDEF/d"     $(OUT) ;\
		[ "$(RCSHELL)" ] && sed -i "s:#!/bin/sh:#!$(RCSHELL):g" out/rc/* out/svc/* ;\
		sed -i "s/    /	/g" $(OUT) ;\
		$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) $(INCLUDE) -o out/os-indications cmd/os-indications.c $(LDFLAGS) ;\
		strip --strip-unneeded -R .comment -R .gnu.version -R .GCC.command.line -R .note.gnu.gold-version out/os-indications ;\
	\
	else \
		echo "LeanInit does not support `uname`!" ;\
		false ;\
	fi
	@
	@# Compile LeanInit, -pthread is used selectively to slightly increase the performance of halt(1)
	@$(CC) $(CFLAGS) -pthread $(CPPFLAGS) $(WFLAGS) $(INCLUDE) -o out/leaninit cmd/init.c $(LDFLAGS)
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) $(INCLUDE) -o out/leaninit-halt cmd/halt.c $(LDFLAGS)
	@strip --strip-unneeded -R .comment -R .gnu.version -R .GCC.command.line -R .note.gnu.gold-version out/leaninit out/leaninit-halt
	@echo "Successfully built LeanInit!"

# Install LeanInit's man pages and license
install-universal:
	@if [ ! -d out ]; then echo 'Please build LeanInit before installing either the RC system or LeanInit itself!'; false; fi
	@mkdir -p "$(DESTDIR)/usr/share/licenses/leaninit"
	@cp -r out/man "$(DESTDIR)/usr/share"
	@install -Dm0644 LICENSE "$(DESTDIR)/usr/share/licenses/leaninit/MIT"

# Install only the base of LeanInit (init, halt and os-indications)
# cd is used with ln(1) for POSIX-compliant relative symlinks (many ln implementations do not have -r)
install-base: install-universal
	@mkdir -p "$(DESTDIR)/sbin"
	@cd "$(DESTDIR)/usr/share/man/man8" ;\
	[ -f leaninit-poweroff.8 ] || ln -sf leaninit-halt.8 leaninit-poweroff.8 ;\
	[ -f leaninit-reboot.8 ] || ln -sf leaninit-halt.8 leaninit-reboot.8
	@install -Dm0755 out/leaninit out/leaninit-halt "$(DESTDIR)/sbin"
	@if [ `uname` = Linux ] || [ `uname` = FreeBSD ]; then \
		install -Dm0755 out/os-indications "$(DESTDIR)/sbin" ;\
	fi
	@cd "$(DESTDIR)/sbin"; ln -sf leaninit-halt leaninit-poweroff
	@cd "$(DESTDIR)/sbin"; ln -sf leaninit-halt leaninit-reboot
	@echo "Successfully installed the base of LeanInit!"

# Install LeanInit RC for use with other init systems (symlink /etc/leaninit/rc to /etc/rc for this to take effect)
install-rc: install-universal
	@mkdir -p "$(DESTDIR)/sbin" "$(DESTDIR)/etc/leaninit/rc.conf.d" "$(DESTDIR)/var/log/leaninit" \
		"$(DESTDIR)/var/lib/leaninit/types" "$(DESTDIR)/var/lib/leaninit/svc"
	@cp -r out/svc "$(DESTDIR)/etc/leaninit"
	@cp -i out/rc.conf.d/* "$(DESTDIR)/etc/leaninit/rc.conf.d" || true
	@cp -i out/rc/rc.conf out/rc/ttys "$(DESTDIR)/etc/leaninit" || true
	@install -Dm0755 out/rc/rc out/rc/rc.svc out/rc/rc.shutdown "$(DESTDIR)/etc/leaninit"
	@install -Dm0755 out/rc/leaninit-service "$(DESTDIR)/sbin"
	@
	@# Enable the default services depending on if the install-flag exists
	@if [ `uname` = FreeBSD ] && [ ! -f "$(DESTDIR)/var/lib/leaninit/install-flag" ]; then \
		touch "$(DESTDIR)/var/lib/leaninit/svc/settings" "$(DESTDIR)/var/lib/leaninit/svc/swap" "$(DESTDIR)/var/lib/leaninit/svc/sysctl" \
			"$(DESTDIR)/var/lib/leaninit/svc/devd" "$(DESTDIR)/var/lib/leaninit/svc/zfs" "$(DESTDIR)/var/lib/leaninit/svc/syslogd" \
			"$(DESTDIR)/var/lib/leaninit/svc/powerd" "$(DESTDIR)/var/lib/leaninit/svc/wpa_supplicant" "$(DESTDIR)/var/lib/leaninit/svc/ntpd" ;\
			"$(DESTDIR)/var/lib/leaninit/svc/cron" ;\
		echo "wpa_supplicant" > "$(DESTDIR)/var/lib/leaninit/types/networking.type" ;\
	\
	elif [ `uname` = NetBSD ] && [ ! -f "$(DESTDIR)/var/lib/leaninit/install-flag" ]; then \
		touch "$(DESTDIR)/var/lib/leaninit/svc/settings" "$(DESTDIR)/var/lib/leaninit/svc/swap" "$(DESTDIR)/var/lib/leaninit/svc/sysctl" \
			"$(DESTDIR)/var/lib/leaninit/svc/powerd" "$(DESTDIR)/var/lib/leaninit/svc/ntpd" \
			"$(DESTDIR)/var/lib/leaninit/svc/networking" "$(DESTDIR)/var/lib/leaninit/svc/cron" ;\
		echo "networking" > "$(DESTDIR)/var/lib/leaninit/types/networking.type" ;\
	\
	elif [ `uname` = Linux ] && [ ! -f "$(DESTDIR)/var/lib/leaninit/install-flag" ]; then \
		touch "$(DESTDIR)/var/lib/leaninit/svc/settings" "$(DESTDIR)/var/lib/leaninit/svc/mountpfs" "$(DESTDIR)/var/lib/leaninit/svc/netface" \
			"$(DESTDIR)/var/lib/leaninit/svc/swap" "$(DESTDIR)/var/lib/leaninit/svc/sysctl" "$(DESTDIR)/var/lib/leaninit/svc/udev" \
			"$(DESTDIR)/var/lib/leaninit/svc/alsa" "$(DESTDIR)/var/lib/leaninit/svc/kmod" ;\
		echo "udev" > "$(DESTDIR)/var/lib/leaninit/types/udev.type" ;\
		cp -i out/rc/rc.banner "$(DESTDIR)/etc/leaninit" ;\
	fi
	@
	@# Finish by creating the install-flag for updates
	@touch "$(DESTDIR)/var/lib/leaninit/install-flag"
	@echo "Successfully installed LeanInit's RC system!"

# Install all of LeanInit (does not overwrite other init systems)
install: install-rc install-base

# Uninstall (only works with normal installations)
uninstall:
	@if [ `id -u` -ne 0 ]; then \
		echo "You must be root to uninstall LeanInit!" ;\
		false ;\
	elif [ ! -x "$(DESTDIR)/sbin/leaninit" ]; then \
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

# Clean the repo
clean:
	@rm -rf out
	@git gc 2> /dev/null
	@git repack > /dev/null 2>&1

# Call clean, then reset the git repo
clobber: clean
	git reset --hard

# Alias for clobber
mrproper: clobber
