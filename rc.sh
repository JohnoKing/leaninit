#!/bin/sh
#
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

# Just in case, set a PATH variable in case /etc/profile does not contain one
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin

# Load /etc/profile
. /etc/profile

# Set the $GETTY variable
GETTY=`grep getty /etc/leaninit/ttys | sed /#/d`

# Set $MODE for getty
DEFLINUX
MODE=38400
ENDEF

DEFBSD
MODE=Pc
ENDEF

# Function which forks processes (for parallel booting)
DEFLINUX
fork() {
	setsid "$@"
}
ENDEF
DEFBSD
fork() {
	daemon "$@"
}
ENDEF

# Echo to the console
DEFLINUX
OUT="/dev/stdout"
ENDEF

DEFBSD
OUT="/dev/console"
ENDEF

print() {
	echo "$@" > $OUT
	echo "$@" >> /var/log/leaninit.log
}

echo "LeanInit is running on `uname -srm`" > $OUT
echo "Checking all filesystems for data corruption..." > $OUT # fsck(8) is run outside of the if statement

# Check all the filesystems in /etc/fstab for damage, and repair them if needed
DEFLINUX
fsck -A > $OUT
ENDEF

DEFBSD
fsck -F > $OUT
ENDEF

# Mount drives and datasets
echo "Remounting root as read-write..." > $OUT
if [ -r /etc/leaninit/svce/zfs ]; then
	. /etc/leaninit/svce/zfs
	main
else
	mount -o remount,rw /
	mount -a
fi

swapon -a
mv /var/log/leaninit.log /var/log/leaninit.log.old
echo "LeanInit is running on `uname -srm`" > /var/log/leaninit.log

# Launch eudev or devd, depending on platform
DEFLINUX
print "Starting udev..."
udevd --daemon
ENDEF

DEFBSD
print "Starting devd..."
fork devd -q
ENDEF

# Manually mount all drives

# Load all sysctl settings
print "Loading settings with sysctl..."
DEFLINUX
fork sysctl --system > $OUT
ENDEF

DEFBSD
fork sysctl -f /etc/sysctl.conf > $OUT
ENDEF

DEFLINUX
# Load all modules specified in the /etc/modules-load.d folder (Linux only)
if [ -r /etc/modules-load.d ]; then
	print "Loading kernel modules..."
	for i in `cat /etc/modules-load.d/* | sed /#/d`; do
		modprobe $i
	done
fi
ENDEF

# Set the hostname
if [ -r /etc/hostname ] && [ "`cat /etc/hostname`" != "" ]; then
	print "Setting the hostname to `cat /etc/hostname`..."
	fork hostname `cat /etc/hostname`
fi

# Set the keyboard layout when needed (this requires kbd to work)
if [ -r /etc/leaninit/kbd.conf ] && [ "`cat /etc/leaninit/kbd.conf`" != "" ]; then
	print "Setting the keyboard layout to `cat /etc/leaninit/kbd.conf`..."
DEFLINUX
	fork loadkeys `cat /etc/leaninit/kbd.conf`
ENDEF
DEFBSD
	fork setxkbmap `cat /etc/leaninit/kbd.conf`
ENDEF
fi

# Start the services
for i in `ls /etc/leaninit/svce | grep -v zfs`; do
	. /etc/leaninit/svce/$i
	export NAME
	print "Starting $NAME..."
	main > $OUT
	echo "Started $NAME" >> /var/log/leaninit.log
done

# Open some gettys, reserving tty7 for the X server and Wayland
print "Launching gettys specified in /etc/leaninit/ttys..."
for i in `cat /etc/leaninit/ttys | sed '/#/d' | sed '/getty/d'`; do
	fork sh -c "while true; do $GETTY $MODE $i; done"
done

# Launch a getty on tty1 without forking
print "Launching a getty on tty1..."

while true; do
DEFLINUX
	$GETTY $MODE tty1
ENDEF

DEFBSD
	$GETTY $MODE ttyv1
ENDEF
done
