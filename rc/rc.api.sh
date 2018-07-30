#!/bin/sh
#
# Copyright (c) 2018 Johnothan King. All rights reserved.
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
# /etc/leaninit/rc.api - Functions and variables for execution of LeanInit scrips
#

# Just in case, set a PATH variable in case /etc/profile does not contain one
PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin

# Load /etc/profile
. /etc/profile

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

# Send the output to the log as well as stdout
print() {
        echo "$@"
        echo "$@" >> /var/log/leaninit.log
}

# Wait for another service if it is enabled
waitforinit() {
	if [ -r /etc/leaninit/svce/$1 ] && [ "$4" = init ]; then
		while true; do
			if [ "`pgrep -x $2`" != "" ]; then
				$3
				break
			fi
		done
	fi
}
