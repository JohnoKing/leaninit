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
# /etc/leaninit.d/rc.svc - Functions and variables for execution of LeanInit scrips
#

# Load rc.svc
. /etc/leaninit.d/rc.svc

# Usage info
usage() {
	echo "Usage: $0 service-name action ..."
	echo "Potential actions:"
	echo "  start"
	echo "  stop"
	echo "  restart"
	echo "  enable"
	echo "  disable"
	echo "  status"
	exit 1
}

# Exit when not given proper arguments
if [ ! -n "$2" ]; then
	usage
elif [ ! -r "/etc/leaninit.d/svc.d/$1" ]; then
	print "The service '$1' does not exist" nolog $RED
	usage
fi

# Execute the service directly
exec /etc/leaninit.d/svc.d/$1 $2
