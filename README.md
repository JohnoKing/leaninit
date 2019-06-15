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

To boot into LeanInit, add `init=/sbin/leaninit` to your kernel command
line. Make sure eudev and iproute2 are installed, as they are required on Linux.

### FreeBSD
Follow the same steps as on Linux to build LeanInit. The Makefile
is compatible with both GNU and BSD Make, so you don't need to install
`gmake` as a separate build dependency.
To boot from LeanInit, append the following line to `/boot/loader.conf`:
`init_path="/sbin/leaninit"`

## Usage
Most information on LeanInit is located in its man pages. 
To read the main man page, run `man leaninit`.
