# Copyright (c) 2017 Johnothan King. All rights reserved.
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

CC := cc
WFLAGS  := -Wall -Wextra -Wpedantic
CFLAGS  := -O2 -ffast-math -fomit-frame-pointer -fstack-protector-strong -fstack-check -fPIC -flto
LDFLAGS := -Wl,--sort-common,--as-needed,-O1,-z,relro,-z,now $(CFLAGS)

all: init

init:
	$(CC) $(WFLAGS) $(CFLAGS) $(CPPFLAGS) -o init init.c $(LDFLAGS)

install: all
	mkdir -p $(DESTDIR)/sbin $(DESTDIR)/etc/leaninit/svce
	install -Dm0755 init $(DESTDIR)/sbin/l-init
	install -Dm0755 halt $(DESTDIR)/sbin/l-halt
	install -Dm0755 reboot $(DESTDIR)/sbin/l-reboot
	install -Dm0755 lsvc $(DESTDIR)/sbin
	install -Dm0755 rc $(DESTDIR)/etc/leaninit
	install -Ddm0755 svc $(DESTDIR)/etc/leaninit
	install -Dm0644 ttys $(DESTDIR)/etc/leaninit
	ln -sr $(DESTDIR)/sbin/l-halt $(DESTDIR)/sbin/l-poweroff
	sed -i 's:/etc/ttys:/etc/leaninit/ttys:g' $(DESTDIR)/etc/leaninit/rc
	sed -i 's:exec init:exec l-init:g' $(DESTDIR)/sbin/l-halt
	sed -i 's:exec halt:exec l-halt:g' $(DESTDIR)/sbin/l-reboot

clean:
	rm -f init

clobber: clean

mrproper: clean
