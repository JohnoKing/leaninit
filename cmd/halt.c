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
 * halt -- Shutdown, reboot or halt the system
 */

#include <leaninit.h>

int main(int argc, char *argv[])
{
    // Halt can only be run by root
    if unlikely (getuid() != 0) {
        printf(RED "* Permission denied" RESET "\n");
        return 1;
    }

    // Long options for halt
    struct option halt_long_options[] = {
#ifndef NetBSD
        { "firmware-setup", no_argument, NULL, 'F' },
#endif
        { "force", no_argument, NULL, 'f' },
        { "halt", no_argument, NULL, 'h' },
        { "no-wall", no_argument, NULL, 'l' },
        { "poweroff", no_argument, NULL, 'p' },
        { "reboot", no_argument, NULL, 'r' },
        { "help", no_argument, NULL, '?' },
        { NULL, 0, NULL, 0 }
    };

    // Variables
    int signal; // Used for safely handling the signal sent to init
    bool wall = true;
    bool osin = false;
#ifdef NetBSD
    const char *opts = "fhlpqr?";
    bool force = true; // Runlevels on NetBSD are buggy
#else
    const char *opts = "Ffhlpqr?";
    bool force = false;
#endif

    // Set the signal to send to init(8) using __progname, while also allowing prefixed names (e.g. leaninit-reboot)
    if unlikely (strstr(__progname, "halt") != 0) // Halt
        signal = SIGUSR1;
    else if likely (strstr(__progname, "poweroff") != 0) // Poweroff
        signal = SIGUSR2;
    else if likely (strstr(__progname, "reboot") != 0) // Reboot
        signal = SIGINT;

    // __progname is not valid
    else {
        printf(RED "* You cannot run halt as %s" RESET "\n", __progname);
        return 1;
    }

    // Parse the given options
    int args;
    while ((args = getopt_long(argc, argv, opts, halt_long_options, NULL)) != -1)
        switch (args) {

            // Display usage info
            case '?':
                printf("Usage: %s [-%s]\n"
#ifndef NetBSD
                       "  -F, --firmware-setup  Reboot into the firmware setup\n"
#endif
                       "  -f, -q, --force       Do not send a signal to init, call sync(2) reboot(2) directly\n"
                       "  -h, --halt            Force halt, even when called as poweroff or reboot\n"
                       "  -l, --no-wall         Turn off wall messages\n"
                       "  -p, --poweroff        Force poweroff, even when called as halt or reboot\n"
                       "  -r, --reboot          Force reboot, even when called as halt or poweroff\n"
                       "  -?, --help            Show this usage information\n",
                       __progname, opts);
                return 1;

            // Skip sending a signal to init(8)
            case 'f':
            case 'q':
                force = true;
                break;

#ifndef NetBSD
            // Firmware setup
            case 'F':
                osin = true;
                break;
#endif

            // Force halt
            case 'h':
                signal = SIGUSR1;
                break;

            // Turn off wall messages
            case 'l':
                wall = false;
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

    // Syslog
    if likely (wall) {
        openlog(__progname, LOG_CONS, LOG_AUTH);
        syslog(LOG_CRIT, "The system is going down NOW!");
        closelog();
    }

#ifdef NetBSD
    // As signaling init will not work reliably on NetBSD, run rc.shutdown NOW
    sync();
    pid_t child = vfork(); // vfork(2) is faster than fork(2) and posix_spawn(3)
    if (child == 0)
        return execve("/etc/leaninit/rc.shutdown", (char *[]) { "rc.shutdown", NULL }, environ);
    else if unlikely (child == -1) {
        perror(RED "* fork() failed with" RESET);
        printf(RED "* Issuing SIGCONT, SIGTERM and SIGKILL unsafely to all processes" RESET "\n");
        kill(-1, SIGCONT);
        kill(-1, SIGTERM);
        sleep(1);
        kill(-1, SIGKILL);
    }

    wait(NULL);
    __builtin_unreachable();

#else
    // Run os-indications if --firmware-setup was passed
    if (osin) {
        pid_t child = vfork(); // vfork(2) is faster than fork(2) and posix_spawn(3)
        if (child == 0)
            return execve("/sbin/os-indications", (char *[]) { "os-indications", "-q", NULL }, environ);
        else if unlikely (child == -1) {
            perror(RED "* fork() failed with" RESET);
            printf(RED "* OsIndications will not be set" RESET "\n");
        }

        wait(NULL);
    }
#endif

    // Skip init if force is true (default for NetBSD)
    if (force) {
        sync(); // Always call sync(2)

        switch (signal) {
            case SIGUSR1: // Halt
                return reboot(SYS_HALT);
            case SIGUSR2: // Poweroff
                return reboot(SYS_POWEROFF);
            case SIGINT: // Reboot
                return reboot(SYS_REBOOT);
        }
    }

    // Send the correct signal to init
    return kill(1, signal);
}
