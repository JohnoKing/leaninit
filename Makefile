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
CC       := cc
SED      := sed
STRIP    := strip
INSTALL  := install
CFLAGS   := -O2 -ffast-math -fomit-frame-pointer -fPIC -pipe
WFLAGS   := -Wall -Wextra -Wpedantic
LDFLAGS  := -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now
LIBS     := -lpthread -lutil
RC       := out/*/*
MANPAGES := $(DESTDIR)/usr/share/man/man5/leaninit-rc.conf.5 $(DESTDIR)/usr/share/man/man5/leaninit-ttys.5 $(DESTDIR)/usr/share/man/man8/leaninit-rc.svc.8 \
	$(DESTDIR)/usr/share/man/man8/leaninit.8 $(DESTDIR)/usr/share/man/man8/leaninit-halt.8 $(DESTDIR)/usr/share/man/man8/leaninit-rc.8 \
	$(DESTDIR)/usr/share/man/man8/leaninit-rc.shutdown.8 $(DESTDIR)/usr/share/man/man8/leaninit-service.8 $(DESTDIR)/usr/share/man/man8/leaninit-poweroff.8 \
	$(DESTDIR)/usr/share/man/man8/leaninit-reboot.8 $(DESTDIR)/usr/share/man/man8/leaninit-zzz.8 $(DESTDIR)/usr/share/man/man8/os-indications.8

# Compile LeanInit
all: clean
	@mkdir -p out
	@cp -r rc out/rc
	@cp -r svc/universal out/svc
	@cp cmd/service.sh out/rc/leaninit-service
	@if [ `uname` = FreeBSD ]; then \
		cp -r svc/freebsd/* out/svc ;\
		$(SED) -i '' "/#DEF Linux/,/#ENDEF/d" $(RC) ;\
		$(SED) -i '' "/#DEF FreeBSD/d" $(RC) ;\
		$(SED) -i '' "/#ENDEF/d"       $(RC) ;\
		$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/os-indications cmd/os-indications.c $(LDFLAGS) $(LIBS) -lefivar ;\
	elif [ `uname` = Linux ]; then \
		cp -r svc/linux/* out/svc ;\
		$(SED) -i "/#DEF FreeBSD/,/#ENDEF/d" $(RC) ;\
		$(SED) -i "/#DEF Linux/d" $(RC) ;\
		$(SED) -i "/#ENDEF/d"     $(RC) ;\
		$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/os-indications cmd/os-indications.c $(LDFLAGS) $(LIBS) ;\
	else \
		echo "`uname` is not supported by LeanInit!" ;\
		false ;\
	fi
	@$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/leaninit      cmd/init.c $(LDFLAGS) $(LIBS)
	@$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/leaninit-halt cmd/halt.c $(LDFLAGS) $(LIBS)
	@$(STRIP) --strip-unneeded -R .comment -R .gnu.version -R .GCC.command.line -R .note.gnu.gold-version out/leaninit out/leaninit-halt out/os-indications
	@echo "Successfully built LeanInit!"

# Install LeanInit (compatible with other init systems)
install:
	@if [ ! -d out ]; then echo 'Please build LeanInit before attempting `make install`'; false; fi
	@mkdir -p  $(DESTDIR)/sbin $(DESTDIR)/etc/leaninit/svc.e $(DESTDIR)/usr/share/licenses/leaninit $(DESTDIR)/var/log $(DESTDIR)/var/run/leaninit
	@cp -r out/svc $(DESTDIR)/etc/leaninit
	@cp -r man $(DESTDIR)/usr/share
	@$(INSTALL) -Dm0644 LICENSE $(DESTDIR)/usr/share/licenses/leaninit/MIT
	@if [ ! -r $(DESTDIR)/etc/leaninit/rc.conf ]; then \
		$(INSTALL) -Dm0644 out/rc/rc.conf $(DESTDIR)/etc/leaninit ;\
	fi
	@if [ ! -r $(DESTDIR)/etc/leaninit/ttys ]; then \
		$(INSTALL) -Dm0644 out/rc/ttys $(DESTDIR)/etc/leaninit ;\
	fi
	@$(INSTALL) -Dm0755 out/rc/rc out/rc/rc.svc out/rc/rc.shutdown $(DESTDIR)/etc/leaninit
	@cd $(DESTDIR)/usr/share/man/man8 ;\
	[ -r leaninit-poweroff.8 ] || ln -sf leaninit-halt.8 leaninit-poweroff.8 ;\
	[ -r leaninit-reboot.8 ] || ln -sf leaninit-halt.8 leaninit-reboot.8 ;\
	if [ `uname` = Linux ]; then [ -r leaninit-zzz.8 ] || ln -sf leaninit-halt.8 leaninit-zzz.8; fi
	@$(INSTALL) -Dm0755 out/leaninit out/leaninit-halt out/os-indications out/rc/leaninit-service $(DESTDIR)/sbin
	@cd $(DESTDIR)/sbin; ln -sf leaninit-halt leaninit-poweroff
	@cd $(DESTDIR)/sbin; ln -sf leaninit-halt leaninit-reboot
	@if [ `uname` = Linux ]; then cd $(DESTDIR)/sbin; ln -sf leaninit-halt leaninit-zzz; fi
	@echo "Successfully installed LeanInit!"

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
