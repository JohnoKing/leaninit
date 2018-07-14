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
be built, named `l-init`. Running `make install` as root will install 
LeanInit, without overriding other init systems.

To boot into LeanInit, add `init=/sbin/l-init` to your kernel command
line.

### FreeBSD
Follow the Linux building steps, but use `gmake` instead of `make` (BSD 
Make is not supported). 
To boot into LeanInit, append the following line to `/boot/loader.conf`:
`init_path="/sbin/l-init:/sbin/init:/rescue/init"`

### Override Build
If you wish, running `make override` and `make override-install` will
build a copy of LeanInit that will override existing init systems once
installed. This is experimental, and not a recommended method of
installation

## Usage

### ACPI Support
To shut down your system when booted into LeanInit, run `l-halt` or
`halt` (depending on your method of installation). `l-poweroff` has
the same function as `l-halt`, while `l-reboot` will restart your
system.
On Linux, running `l-init 7` will cause your system to hibernate.

### Enabling and disabling services
The command to use for enabling and disabling services is `lsvc` (on both 
normal and override installs). To enable a service, use `lsvc -e yourservice`; 
and to disable a service, use `lsvc -d yourservice`. A list of services 
LeanInit includes scripts for can be found [here](https://github.com/JohnoKing/leaninit/tree/master/svc).

### /etc/leaninit/ttys
LeanInit has a config file that allows you to add or remove ttys from 
being launched, and change the getty program used on boot.
