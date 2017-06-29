LeanInit
========
_A minimal init system_

## What is this
This is a small init system that I made for myself, written from scratch. 
I was motivated to make this init system, because when I was cleaning up 
the [v7init](https://github.com/JohnoKing/v7init) (my port of the UNIX V7 init system to Linux), I found there was so much useless 
code I would be better off rewriting an init system from scratch, using a 
whole new code base. In addition to allowing me to write a much faster init 
system, I could put it under the much more permissive MIT license, rather 
than the 4-clause BSD license v7init was under; as LeanInit contains none 
of the UNIX code at all.

## Building/Installing
Run `make` and a LeanInit binary compatible with other init systems will 
be built, named `l-init`. Running `make install` as root will install 
LeanInit, without overriding other init systems. To boot from LeanInit, 
add `init=/sbin/l-init` to your kernel command line. If you are using 
agetty as your getty program, you can either replace the `getty` line in 
/etc/leaninit/ttys with `agetty`, or make /sbin/getty a symlink to agetty.

Alternatively, run `make override` to make a LeanInit binary without 
regard for other init systems; and running `make override_install` will 
install LeanInit, overriding any other init systems in the process. If 
you are using agetty as your getty program, you can either replace the 
`getty` line in /etc/ttys with `agetty`, or make /sbin/getty a symlink 
to agetty.

## Usage

### Shutting down your system
To shut down your system when booted into LeanInit, run `l-halt` (if you 
installed LeanInit normally) or `halt` (if you installed LeanInit with 
`make override`) and the system will either turn off or halt, depending 
on whether or not your system supports power management (ACPI). The 
`l-poweroff` is just a symlink to `l-halt` (normal install), while 
`poweroff` is a symlink to `halt` (override install).

### Restarting your system
To restart your system, run either `l-reboot` or `reboot`, depending on 
how you installed LeanInit.

### Enabling and disabling services
The command to use for enabling and disabling services is lsvc (on both 
normal and override installs). To enable a service, use `lsvc -e yourservice`; 
and to disable a service, use `lsvc -d yourservice`. A list of services 
LeanInit includes scripts for can be found [here](https://github.com/JohnoKing/leaninit/tree/master/svc).

### /etc/ttys
LeanInit has a config file that allows you to add or remove ttys from 
being launched, and change the getty program used. On a normal install, 
it is /etc/leaninit/ttys; and on an override install, it is /etc/ttys.
