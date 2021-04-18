/*
 * Copyright © 2018-2021 Johnothan King. All rights reserved.
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

#if defined(__TINYC__)
#undef _FORTIFY_SOURCE // Silence warnings
#endif

// To get POSIX_SPAWN_SETSID on Linux, define _GNU_SOURCE
#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

// Include files
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <signal.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <syslog.h>
#include <unistd.h>
#if defined(__FreeBSD__)
#include <efivar.h>
#endif

// LeanInit's version number
#define VERSION_NUMBER "v8.0.1"

// OS specific macros
#if defined(__linux__)
#define DEFAULT_TTY  "/dev/tty1"
#define SYS_POWEROFF RB_POWER_OFF
#define SYS_REBOOT   RB_AUTOBOOT
#define SYS_HALT     RB_HALT_SYSTEM
#elif defined(__FreeBSD__)
#define DEFAULT_TTY  "/dev/ttyv0"
#define SYS_POWEROFF RB_POWEROFF
#define SYS_REBOOT   RB_AUTOBOOT
#define SYS_HALT     RB_HALT
#elif defined(__NetBSD__)
#define DEFAULT_TTY  "/dev/constty"
#define SYS_POWEROFF RB_POWERDOWN, NULL
#define SYS_REBOOT   RB_AUTOBOOT, NULL
#define SYS_HALT     RB_HALT, NULL
#endif

// Colors
#define RESET  "\x1b[m"
#define RED    "\x1b[1;31m"
#define GREEN  "\x1b[1;32m"
#define YELLOW "\x1b[1;33m"
#define BLUE   "\x1b[1;34m"
#define PURPLE "\x1b[1;35m"
#define CYAN   "\x1b[1;36m"
#define WHITE  "\x1b[1;37m"

// External char variables
extern char *__progname; // argv[0] is not sufficient
extern char **environ;   // This is used with execve(2)

/* These are attributes and macros used for compiler optimization. Note
   that `__has_builtin` only works in Clang and GCC 10+. */
#if !defined(__has_builtin)
#define __has_builtin(x) 0
#endif
#define cold   __attribute__((__cold__))
#define unused __attribute__((__unused__))
#if __has_builtin(__builtin_expect_with_probability)
#define likely(x)        (__builtin_expect_with_probability((x), 1, 0.9))
#define unlikely(x)      (__builtin_expect_with_probability((x), 0, 0.8))
#define very_unlikely(x) (__builtin_expect_with_probability((x), 0, 0.9))
#elif defined(__TINYC__)
// tcc doesn't have many of the GCC builtins
#define likely(x)        (x)
#define unlikely(x)      (x)
#define very_unlikely(x) (x)
#define __builtin_unreachable()
#else
#define likely(x)        (__builtin_expect((x), 1))
#define unlikely(x)      (__builtin_expect((x), 0))
#define very_unlikely(x) (__builtin_expect((x), 0))
#endif
