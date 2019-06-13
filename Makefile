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
MANPAGES := $(DESTDIR)/usr/share/man/man5/lrc.conf.5 $(DESTDIR)/usr/share/man/man5/lttys.5 $(DESTDIR)/usr/share/man/man8/rc.svc.8 \
	$(DESTDIR)/usr/share/man/man8/leaninit.8 $(DESTDIR)/usr/share/man/man8/lhalt.8 $(DESTDIR)/usr/share/man/man8/lrc.8 \
	$(DESTDIR)/usr/share/man/man8/lrc.shutdown.8 $(DESTDIR)/usr/share/man/man8/lservice.8 $(DESTDIR)/usr/share/man/man8/lpoweroff.8 \
	$(DESTDIR)/usr/share/man/man8/lreboot.8 $(DESTDIR)/usr/share/man/man8/lzzz.8 $(DESTDIR)/usr/share/man/man8/os-indications.8

# Compile LeanInit
all: clean
	@mkdir -p  out
	@cp -r rc  out/rc
	@cp -r svc/universal out/svc
	@cp cmd/service.sh   out/rc/lservice
	@if [ `uname` = FreeBSD ]; then \
		cp -r svc/freebsd/* out/svc ;\
		$(SED) -i '' "/#DEF Linux/,/#ENDEF/d" $(RC) ;\
		$(SED) -i '' "/#DEF FreeBSD/d" $(RC) ;\
		$(SED) -i '' "/#ENDEF/d"       $(RC) ;\
		$(SED) -i '' "s:TTY:ttyv:g"  out/rc/ttys ;\
		$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/os-indications cmd/os-indications.c $(LDFLAGS) $(LIBS) -lefivar ;\
	elif [ `uname` = Linux ]; then \
		cp -r svc/linux/* out/svc ;\
		$(SED) -i "/#DEF FreeBSD/,/#ENDEF/d" $(RC) ;\
		$(SED) -i "/#DEF Linux/d" $(RC) ;\
		$(SED) -i "/#ENDEF/d"     $(RC) ;\
		$(SED) -i "s:TTY:tty:g" out/rc/ttys ;\
		$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/os-indications cmd/os-indications.c $(LDFLAGS) $(LIBS) ;\
	else \
		echo "`uname` is not supported by LeanInit!" ;\
		false ;\
	fi
	@$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/leaninit cmd/init.c   $(LDFLAGS) $(LIBS)
	@$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/lhalt    cmd/halt.c   $(LDFLAGS) $(LIBS)
	@$(CC) $(CFLAGS) $(WFLAGS) -D`uname` -o out/lgetty   cmd/lgetty.c $(LDFLAGS) $(LIBS)
	@$(STRIP) --strip-unneeded -R .comment -R .gnu.version -R .GCC.command.line -R .note.gnu.gold-version out/leaninit out/lhalt out/lgetty out/os-indications
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
	[ -r lpoweroff.8 ] || ln -sf lhalt.8 lpoweroff.8 ;\
	[ -r lreboot.8 ] || ln -sf lhalt.8 lreboot.8 ;\
	if [ `uname` = Linux ]; then [ -r lzzz.8 ] || ln -sf lhalt.8 lzzz.8; fi
	@$(INSTALL) -Dm0755 out/leaninit out/lhalt out/os-indications out/rc/lservice out/lgetty $(DESTDIR)/sbin
	@cd $(DESTDIR)/sbin; ln -sf lhalt lpoweroff
	@cd $(DESTDIR)/sbin; ln -sf lhalt lreboot
	@if [ `uname` = Linux ]; then cd $(DESTDIR)/sbin; ln -sf lhalt lzzz; fi
	@echo "Successfully installed LeanInit!"

# Upgrade a LeanInit v1 install to v2 (this will be removed in v2.1.0)
upgrade:
	@if [ `id -u` != 0 ]; then \
		echo "You must be root to upgrade LeanInit!" ;\
		false ;\
	fi
	@if [ ! -x $(DESTDIR)/sbin/leaninit ]; then \
		echo "Failed to detect an installation of LeanInit, exiting..." ;\
		false ;\
	fi
	@if [ ! -d out ]; then echo 'Please build LeanInit before attempting `make upgrade`'; false; fi
	@rm -rf $(MANPAGES) $(DESTDIR)/etc/leaninit/svc.d $(DESTDIR)/etc/leaninit/svc.e/fstrim $(DESTDIR)/etc/leaninit/svc.e/consolekit
	@cp -r out/svc $(DESTDIR)/etc/leaninit
	@cp -r man $(DESTDIR)/usr/share
	@$(INSTALL) -Dm0644 LICENSE $(DESTDIR)/usr/share/licenses/leaninit/MIT
	@if [ ! -r $(DESTDIR)/etc/leaninit/rc.conf ]; then \
		$(INSTALL) -Dm0644 out/rc/rc.conf $(DESTDIR)/etc/leaninit ;\
	fi
	@$(INSTALL) -Dm0644 out/rc/ttys $(DESTDIR)/etc/leaninit ;\
	@$(INSTALL) -Dm0755 out/rc/rc out/rc/rc.svc out/rc/rc.shutdown $(DESTDIR)/etc/leaninit
	@cd $(DESTDIR)/usr/share/man/man8 ;\
	[ -r lpoweroff.8 ] || ln -sf lhalt.8 lpoweroff.8 ;\
	[ -r lreboot.8 ] || ln -sf lhalt.8 lreboot.8 ;\
	if [ `uname` = Linux ]; then [ -r lzzz.8 ] || ln -sf lhalt.8 lzzz.8; fi
	@$(INSTALL) -Dm0755 out/leaninit out/lhalt out/os-indications out/rc/lservice out/lgetty $(DESTDIR)/sbin
	@cd $(DESTDIR)/sbin; ln -sf lhalt lpoweroff
	@cd $(DESTDIR)/sbin; ln -sf lhalt lreboot
	@if [ `uname` = Linux ]; then cd $(DESTDIR)/sbin; ln -sf lhalt lzzz; fi
	@rm -f $(DESTDIR)/var/log/leaninit.log.2 $(DESTDIR)/var/log/leaninit.log.old.2 $(DESTDIR)/sbin/lrunlevel $(DESTDIR)/sbin/lgetty $(DESTDIR)/etc/leaninit/svc.e/getty.type $(DESTDIR)/etc/leaninit/svc.e/lgetty
	@echo "Successfully upgraded LeanInit v1 to v2!"

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
	@rm -rf $(DESTDIR)/sbin/leaninit $(DESTDIR)/sbin/lhalt $(DESTDIR)/sbin/lpoweroff $(DESTDIR)/sbin/lreboot $(DESTDIR)/sbin/os-indications $(DESTDIR)/usr/share/licenses/leaninit \
		$(DESTDIR)/sbin/lzzz $(DESTDIR)/sbin/lservice $(DESTDIR)/etc/leaninit $(DESTDIR)/var/log/leaninit.log $(DESTDIR)/var/run/leaninit $(MANPAGES)
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
