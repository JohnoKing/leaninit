LeanInit
========
_A fast init system_

## What is this
This is a speedy BSD-style init system for Linux, FreeBSD and (still a WIP) NetBSD.

## Building and Installing
### Linux
Run `make` will compile LeanInit (compiled files are placed in `./out`).
Running `make install` as root will install LeanInit, without overriding other existing init systems.
You can change the destination LeanInit is installed to by using `$DESTDIR`.

To boot into LeanInit, add `init=/sbin/leaninit` to your kernel command line.
Make sure eudev and iproute2 are installed, as they are required on Linux.

### FreeBSD
Follow the same steps as on Linux to build LeanInit.
The Makefileis compatible with both GNU and BSD Make, so you don't need to install `gmake` as a separate build dependency.
To boot from LeanInit, append the following line to `/boot/loader.conf`: `init_path="/sbin/leaninit:/sbin/init"`.  

### NetBSD
As stated above, LeanInit will compile with BSD make.
However, the NetBSD boot loader does not support the `init_path` setting.
As a workaround, you can turn `/sbin/init` into a symlink.
*Only do this at your own risk, as you can only revert this with `chroot` on a resuce disk if something goes wrong.*
If you are still willing to change the init system, the following set of commands will backup `/sbin/init` and change the init system to LeanInit:
`mv /sbin/init /sbin/init.old`  
`ln -s /sbin/leaninit /sbin/init`  

### init(8)-only installation
To install only the implementations of init(8), halt(8), the man pages and the os-indications(8) program,
build LeanInit then run `make install-base`.
Keep in mind that this will not install LeanInit RC, so you must use a different implementation of `/etc/rc`.

### RC-only installation
To install only the init scripts LeanInit uses for use with other BSD-style init systems,
build LeanInit then run `make install-rc`.
For LeanInit RC to be used by init on the next boot, backup the system's current `/etc/rc`
and `/etc/rc.shutdown` scripts, then run the following commands to symlink LeanInit's RC scripts to `/etc`:
`ln -s /etc/leaninit/rc /etc/rc && ln -s /etc/leaninit/rc.shutdown /etc/rc.shutdown`.

### Optimization
The shell LeanInit RC's scripts run under (defined at the start of each script) can be changed with the `$RCSHELL` environment variable.
To compile with BusyBox ash as the default shell (to increase performance), build with the following command:
`make RCSHELL='/bin/busybox ash'`
LeanInit will also net slightly better performance on Linux if statically compiled with musl libc.

## Usage
Most information on LeanInit is located in its man pages.
To read the main man page, run `man leaninit`.
