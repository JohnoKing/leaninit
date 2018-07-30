/*
 * Copyright (c) 2018 Johnothan King. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Include files
#include <sys/reboot.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#ifdef LINUX
#include <utmp.h>
#endif
#ifdef FREEBSD
#include <libutil.h>
#endif

// Power management macros
#define POWEROFF 0
#define REBOOT   6
#define HALT     7

#ifdef LINUX
#define SLEEP    8
#define SYS_POWEROFF RB_POWER_OFF
#define SYS_HALT     RB_HALT_SYSTEM
#define DEFAULT_TTY  "/dev/tty1"
#endif

#ifdef FREEBSD
#define SYS_POWEROFF RB_POWEROFF
#define SYS_HALT     RB_HALT
#define DEFAULT_TTY  "/dev/ttyv0"
#endif

// Macros for service actions
#define ENABLE  10
#define DISABLE 11
#define STOP    12
#define START   13
#define RESTART 14

// argv[0] is not sufficent
extern char *__progname;
