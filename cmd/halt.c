/*
 * Copyright © 2018-2020 Johnothan King. All rights reserved.
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

/*
 * halt - Shutdown, reboot or halt system
 */

#include <leaninit.h>

// Bitmask values
#define FORCE_HALT    1 << 0 // Skip sending a signal to init (also runs rc.shutdown on NetBSD)
#define WALL_MESSAGE  1 << 1 // Output a message to syslog(3)
#ifndef NetBSD
#define OsIndications 1 << 2 // Flag used to run os-indications(8) before rebooting
#endif

int main(int argc, char *argv[])
{
    // Halt can only be run by root
    if(getuid() != 0) {
        printf(RED "* Permission denied" RESET "\n");
        return 1;
    }

    // Long options for halt
    struct option halt_long_options[] = {
#       ifndef NetBSD
        { "firmware-setup", no_argument, 0, 'F' },
#       endif
        { "force",          no_argument, 0, 'f' },
        { "halt",           no_argument, 0, 'h' },
        { "no-wall",        no_argument, 0, 'l' },
        { "poweroff",       no_argument, 0, 'p' },
        { "reboot",         no_argument, 0, 'r' },
        { "help",           no_argument, 0, '?' },
        {  0,                         0, 0,  0  }
    };

    // Int variables
    int signal; // Used for safely handling the signal sent to init
#   ifdef NetBSD
    unsigned char flags = FORCE_HALT | WALL_MESSAGE; // Runlevels on NetBSD are buggy
    const char *opts    = "fhlpqr?";
#   else
    unsigned char flags = WALL_MESSAGE;
    const char *opts    = "fFhlpqr?";
#   endif

    // Set the signal to send to init(8) using __progname, while also allowing prefixed names (e.g. leaninit-reboot)
    if(strstr(__progname,      "halt")     != 0) // Halt
        signal = SIGUSR1;
    else if(strstr(__progname, "poweroff") != 0) // Poweroff
        signal = SIGUSR2;
    else if(strstr(__progname, "reboot")   != 0) // Reboot
        signal = SIGINT;

    // __progname is not valid
    else {
        printf(RED "* You cannot run halt as %s" RESET "\n", __progname);
        return 1;
    }

    // Parse the given options
    int args;
    while((args = getopt_long(argc, argv, opts, halt_long_options, NULL)) != -1) {
        switch(args) {

            // Display usage info
            case '?':
                printf("Usage: %s [-%s]\n", __progname, opts);
                printf("  -f, -q, --force       Do not send a signal to init, call sync(2) reboot(2) directly\n");
#               ifndef NetBSD
                printf("  -F, --firmware-setup  Reboot into the firmware setup\n");
#               endif
                printf("  -h, --halt            Force halt, even when called as poweroff or reboot\n");
                printf("  -l, --no-wall         Turn off wall messages\n");
                printf("  -p, --poweroff        Force poweroff, even when called as halt or reboot\n");
                printf("  -r, --reboot          Force reboot, even when called as halt or poweroff\n");
                printf("  -?, --help            Show this usage information\n");
                return 1;

            // Skip sending a signal to init(8)
            case 'f':
            case 'q':
#               ifndef NetBSD
                flags ^= FORCE_HALT;
#               endif
                break;

            // Firmware setup
#           ifndef NetBSD
            case 'F':
                flags ^= OsIndications;
                break;
#           endif

            // Force halt
            case 'h':
                signal = SIGUSR1;
                break;

            // Turn off wall messages
            case 'l':
                flags ^= WALL_MESSAGE;
                break;

            // Force poweroff
            case 'p':
                signal = SIGUSR2;
                break;

            // Force reboot
            case 'r':
                signal = SIGINT;
                break;
        }
    }

    // Syslog
    if((flags & WALL_MESSAGE) == WALL_MESSAGE) {
        openlog(__progname, LOG_CONS, LOG_AUTH);
        syslog(LOG_CRIT, "The system is going down NOW!");
        closelog();
    }

    // As signaling init will not work reliably on NetBSD, run rc.shutdown NOW
#   ifdef NetBSD
    pid_t child = fork();
    if(child == 0) {
        sync();
        return execve("/etc/leaninit/rc.shutdown", (char*[]){ "rc.shutdown", NULL }, environ);
    } else if(child == -1) {
        perror(RED "* fork() failed with" RESET);
        printf(RED "* Issuing SIGCONT, SIGTERM and SIGKILL unsafely to all processes" RESET "\n");
        kill(-1, SIGCONT);
        kill(-1, SIGTERM);
        sleep(1);
        kill(-1, SIGKILL);
    }

    wait(NULL);

    // Run os-indications if --firmware-setup was passed (Linux and FreeBSD only)
#   else
    if((flags & OsIndications) == OsIndications) {
        pid_t child = fork();
        if(child == 0)
            return execve("/sbin/os-indications", (char*[]){ "os-indications", "-q", NULL }, environ);
        else if(child == -1) {
            perror(RED "* fork() failed with" RESET);
            printf(RED "* OsIndications will not be set" RESET "\n");
        }

        wait(NULL);
    }
#   endif

    // Skip init if force is true (default for NetBSD)
    if((flags & FORCE_HALT) == FORCE_HALT) {
        sync(); // Always call sync(2)

        switch(signal) {
            case SIGUSR1: // Halt
                return reboot(SYS_HALT);
            case SIGUSR2: // Poweroff
                return reboot(SYS_POWEROFF);
            case SIGINT:  // Reboot
                return reboot(SYS_REBOOT);
        }
    }

    // Send the correct signal to init
    return kill(1, signal);
}
