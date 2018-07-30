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

# Source rc.api and zfs.cfg
. /etc/leaninit/rc.api
. /etc/leaninit/zfs.cfg

# Set the $GETTY variable
GETTY=`grep getty /etc/leaninit/ttys | sed '/#/d'`

# Set $MODE for getty
DEFLINUX
MODE=38400
ENDEF

DEFBSD
MODE=Pc
ENDEF

# Check all the filesystems in /etc/fstab for damage, and repair them if needed
echo "Checking all filesystems for data corruption..."
DEFLINUX
fsck -A
ENDEF

DEFBSD
fsck -F
ENDEF

# Mount drives and datasets
echo "Remounting root as read-write..."
if [ "$ZFS_ENABLE" = "TRUE" ]; then
	# Mount everything
	zfs mount -a

	# ZFS NFS
	zfs share -a

	# Turn on writes
	for z in `zpool list -H | awk '{print $1;}'`; do
		print "Turning readonly off for dataset $z"
		zfs readonly=off $z
	done
else
	mount -o remount,rw /
fi
mount -a
swapon -a

# Logging
touch /var/log/leaninit.log
mv /var/log/leaninit.log /var/log/leaninit.log.old
echo "LeanInit is running on `uname -srm`" > /var/log/leaninit.log

# Launch eudev (Linux)
DEFLINUX
print "Starting udev..."
udevd --daemon
ENDEF

# Load all sysctl settings
print "Loading settings with sysctl..."
DEFLINUX
fork sysctl --system
ENDEF

DEFBSD
fork sysctl -f /etc/sysctl.conf
ENDEF

DEFLINUX
# Load all modules specified in the /etc/modules-load.d folder (Linux only)
if [ -d /etc/modules-load.d ]; then
	print "Loading kernel modules..."
	for m in `cat /etc/modules-load.d/* | sed '/#/d'`; do
		modprobe $m
	done
fi
ENDEF

# Set the hostname
if [ -r /etc/hostname ] && [ "`cat /etc/hostname`" != "" ]; then
	print "Setting the hostname to `cat /etc/hostname`..."
	fork hostname `cat /etc/hostname`
fi

# Set the keyboard layout when needed (this requires kbd to work)
if [ -r /etc/leaninit/kbd.cfg ] && [ "`cat /etc/leaninit/kbd.cfg`" != "" ]; then
	print "Setting the keyboard layout to `cat /etc/leaninit/kbd.cfg`..."
DEFLINUX
	fork loadkeys `cat /etc/leaninit/kbd.cfg`
ENDEF
DEFBSD
	fork setxkbmap `cat /etc/leaninit/kbd.cfg`
ENDEF
fi

# Start the services
for sv in `ls /etc/leaninit/svce`; do
	fork sh /etc/leaninit/svc-start $sv
done

# Open gettys on the ttys specified in /etc/leaninit/ttys
print "Launching gettys specified in /etc/leaninit/ttys..."
for tty in `cat /etc/leaninit/ttys | sed '/#/d' | sed '/getty/d'`; do
	fork sh -c "while true; do $GETTY $MODE $tty; done"
done

# Launch a getty on tty1 without forking
print "Launching a getty on tty1..."
while true; do
DEFLINUX
	$GETTY $MODE tty1
ENDEF

DEFBSD
	$GETTY $MODE ttyv0
ENDEF
done
