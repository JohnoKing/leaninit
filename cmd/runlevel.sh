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
# runlevel - Prints the current and previous runlevels (or unknown if there is none available).
#    This script uses the $RUNLEVEL and $PREVLEVEL variables to determine the current runlevel.
#

if [ -z "$PREVLEVEL" ] && [ -z "$RUNLEVEL" ]; then
	echo "unknown"
	exit 1
fi

if [ -n "$PREVLEVEL" ]; then
	__PREVLEVEL="$PREVLEVEL"
else
	__PREVLEVEL=N
fi

if [ -n "$RUNLEVEL" ]; then
	__RUNLEVEL="$RUNLEVEL"
else
	__RUNLEVEL=N
fi

# Print the previous and current runlevels
echo "$__PREVLEVEL $__RUNLEVEL"
exit 0
