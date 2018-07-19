LeanInit
========
_A minimal init system_

## What is this
This is a small init system that I made for myself, written from scratch.
I was motivated to make this init system, because when I was cleaning up
[v7init](https://github.com/JohnoKing/v7init) (my port of the UNIX V7 init system to Linux), I found there was so much useless
code I would be better off rewriting an init system from scratch, using a
whole new code base. In addition to allowing me to write a much faster init
system, I could put it under the much more permissive MIT license, rather
than the 4-clause BSD license v7init was under; as LeanInit contains none
of the UNIX code at all.

## Building and Installing
### Linux
Run `make` and a LeanInit binary compatible with other init systems will
be built, named `linit`. Running `make install` as root will install
LeanInit, without overriding other init systems.

To boot into LeanInit, add `init=/sbin/linit` to your kernel command
line. Make sure eudev is installed, as it is required on Linux.

### FreeBSD
Follow the same steps as on Linux to build LeanInit. The Makefile
is compatible with both GNU and BSD Make, so you don't need to install
`gmake` as a seperate build dependency.
To boot from LeanInit, append the following line to `/boot/loader.conf`:
`init_path="/sbin/linit:/sbin/init:/rescue/init"`

### Override Build
If you wish, running `make override` and `make override-install` will
build a copy of LeanInit that will override existing init systems once
installed. This is experimental, and not a recommended method of
installation.

## Usage

### ACPI Support
To shut down your system when booted into LeanInit, run `lhalt` or
`halt` (depending on your method of installation). `lpoweroff` has
the same function as `lhalt`, while `lreboot` will restart your system.
On Linux, running `linit 8` or `lzzz` will cause your system to hibernate.

### Enabling and disabling services
The tool for enabling and disabling services is `lsvc` (on both
normal and override installs). To enable a service, use `lsvc -e yourservice`;
and to disable a service, use `lsvc -d yourservice`. A list of services
LeanInit includes scripts for can be found [here](https://github.com/JohnoKing/leaninit/tree/master/svc).

### /etc/hostname
`/etc/hostname` is the file LeanInit uses to set the system's hostname.

### /etc/leaninit/kbd.conf
`/etc/leaninit/kbd.conf` allows you to change the system's keyboard layout
(kbd must be installed for this to work on Linux).

### /etc/leaninit/ttys
`/etc/leaninit/ttys` allows you to add and remove the ttys launched on boot,
as well as change the getty program that will be used.
