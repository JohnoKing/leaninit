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
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#ifdef FreeBSD
#include <libutil.h>
#else
#include <utmp.h>
#endif

// OS specific macros
#ifdef Linux
#define CONSOLE      "/dev/tty1"
#define SYS_POWEROFF RB_POWER_OFF
#define SYS_HALT     RB_HALT_SYSTEM
#endif
#ifdef FreeBSD
#define CONSOLE      "/dev/ttyv0"
#define SYS_POWEROFF RB_POWEROFF
#define SYS_HALT     RB_HALT
#endif

// Colors
#define RESET  "\x1b[m"
#define RED    "\x1b[91m"
#define GREEN  "\x1b[92m"
#define YELLOW "\x1b[93m"
#define BLUE   "\x1b[94m"
#define PURPLE "\x1b[95m"
#define CYAN   "\x1b[96m"
#define WHITE  "\x1b[97m"

// argv[0] is not sufficent
extern char *__progname;
