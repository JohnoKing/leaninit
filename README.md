LeanInit
========
**The primary repo has been moved to https://gitlab.com/JohnoKing/leaninit**
_A fast init system_

## What is this
This is a small init system that I made for myself, written from scratch.
I was motivated to make this init system, because when I was cleaning up
[v7init](https://gitlab.com/JohnoKing/v7init) (my port of the UNIX V7 init system to Linux), I found there was so much useless
code I would be better off rewriting an init system from scratch, using a
whole new code base. In addition to allowing me to write a much faster init
system, I could put it under the much more permissive MIT license, rather
than the 4-clause BSD license v7init was under; as LeanInit contains none
of the UNIX code at all.

## Building and Installing
### Linux
Run `make` will compile LeanInit (compiled files are placed in `./out`).
Running `make install` as root will install
LeanInit, without overriding other existing init systems. 
You can change the destination LeanInit is installed to by using `$DESTDIR`.

To boot into LeanInit, add `init=/sbin/leaninit` to your kernel command
line. Make sure eudev and iproute2 are installed, as they are required on Linux.

### FreeBSD
Follow the same steps as on Linux to build LeanInit. The Makefile
is compatible with both GNU and BSD Make, so you don't need to install
`gmake` as a seperate build dependency.
To boot from LeanInit, append the following line to `/boot/loader.conf`:
`init_path="/sbin/leaninit"`

## Usage
### Enabling and disabling services
The tool for enabling and disabling services is `lsvc`.
To enable a service, use `lsvc -e yourservice`;
and to disable a service, use `lsvc -d yourservice`. A list of services
LeanInit includes scripts for can be found [here](https://gitlab.com/JohnoKing/leaninit/tree/master/svc.d)
To see extra information on `lsvc`, run `lsvc --help` to see all of the options `lsvc` accepts.

### /etc/leaninit.d/rc.conf
`/etc/leaninit.d/rc.conf` allows you to change your hostname, keyboard layout (Linux), 
wireless interface, and other settings.

### /etc/leaninit.d/ttys
`/etc/leaninit.d/ttys` allows you to add and remove the ttys launched on boot,
as well as change the getty program that will be used.
