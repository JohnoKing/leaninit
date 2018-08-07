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
#ifdef FreeBSD
#include <libutil.h>
#else
#include <utmp.h>
#endif

// OS specific macros
#ifdef Linux
#define SYS_POWEROFF RB_POWER_OFF
#define SYS_HALT     RB_HALT_SYSTEM
#define DEFAULT_TTY  "/dev/tty1"
#endif

#ifdef FreeBSD
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

// Colors
#define COLOR_RESET        "\x1b[m"
#define COLOR_BOLD         "\x1b[1m"
#define COLOR_FAINT        "\x1b[2m"
#define COLOR_ITALIC       "\x1b[3m"
#define COLOR_UNDERLINE    "\x1b[4m"
#define COLOR_BLACK        "\x1b[30m"
#define COLOR_RED          "\x1b[31m"
#define COLOR_GREEN        "\x1b[32m"
#define COLOR_BROWN        "\x1b[33m"
#define COLOR_BLUE         "\x1b[34m"
#define COLOR_PURPLE       "\x1b[35m"
#define COLOR_CYAN         "\x1b[36m"
#define COLOR_LIGHT_GRAY   "\x1b[37m"
#define COLOR_GRAY         "\x1b[90m"
#define COLOR_LIGHT_RED    "\x1b[91m"
#define COLOR_LIGHT_GREEN  "\x1b[92m"
#define COLOR_YELLOW       "\x1b[93m"
#define COLOR_LIGHT_BLUE   "\x1b[94m"
#define COLOR_LIGHT_PURPLE "\x1b[95m"
#define COLOR_LIGHT_CYAN   "\x1b[96m"
#define COLOR_WHITE        "\x1b[97m"

// argv[0] is not sufficent
extern char *__progname;
