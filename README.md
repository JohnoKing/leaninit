LeanInit
========
_A fast init system_

## What is this
This is a speedy BSD-style init system that I created for myself, written from scratch.

## Building and Installing
### Linux
Run `make` will compile LeanInit (compiled files are placed in `./out`).
Running `make install` as root will install
LeanInit, without overriding other existing init systems.
You can change the destination LeanInit is installed to by using `$DESTDIR`.
LeanInit will use `unsigned char` variables for small variables by default
to decrease the amount of RAM LeanInit uses during runtime.
This can be changed to `unsigned int` by building LeanInit with the command
`make CPPFLAGS=-DUINT32 install` to increase performance at the cost of
increased RAM usage.

To boot into LeanInit, add `init=/sbin/leaninit` to your kernel command
line. Make sure eudev and iproute2 are installed, as they are required on Linux.
Also, make sure `/bin/sh` is symlinked to a fast shell such as dash or ksh to
increase performance.

### FreeBSD
Follow the same steps as on Linux to build LeanInit. The Makefile
is compatible with both GNU and BSD Make, so you don't need to install
`gmake` as a separate build dependency.
To boot from LeanInit, append the following line to `/boot/loader.conf`:
`init_path="/sbin/leaninit"`

### RC-only installation
To install only the init scripts LeanInit uses for use with other BSD-style init systems,
build LeanInit then run `make install-rc`.
For LeanInit RC to be used by init on the next boot, backup the system's current `/etc/rc`
and `/etc/rc.shutdown` scripts, then run the following commands to symlink LeanInit's RC scripts to `/etc`:
`ln -s /etc/leaninit/rc /etc/rc && ln -s /etc/leaninit/rc.shutdown /etc/rc.shutdown`.

## Usage
Most information on LeanInit is located in its man pages.
To read the main man page, run `man leaninit`.
