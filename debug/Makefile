# Copyright © 2018-2021 Johnothan King. All rights reserved.
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

# Variables
CC       := cc
CFLAGS   := -Os -fomit-frame-pointer -fpic -fno-plt -pipe
CPPFLAGS := -D_FORTIFY_SOURCE=2
INCLUDE  := -I../include
WFLAGS   := -Wall -Wextra -Wpedantic
LDFLAGS  := -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now

# Compile signal-interfere and stall
all: clean
	@mkdir -p out
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) $(INCLUDE) -o out/signal-interfere signal-interfere.c $(LDFLAGS)
	@$(CC) $(CFLAGS) $(CPPFLAGS) $(WFLAGS) $(INCLUDE) -o out/stall stall.c $(LDFLAGS)
	@strip --strip-unneeded -R .comment -R .gnu.version out/*
	@echo "Successfully built the LeanInit debugging tools!"

# Only clean this directory
clean:
	@rm -rf out

# Aliases for clean
clobber: clean
mrproper: clean
