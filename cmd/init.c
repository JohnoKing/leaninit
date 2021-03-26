/*
 * Copyright © 2017-2020 Johnothan King. All rights reserved.
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
 * leaninit -- A fast init system
 */

#include <leaninit.h>

// Universal variables
#define SINGLE_USER (1 << 0)
#define VERBOSE     (1 << 1)
#define BANNER      (1 << 2)
static unsigned char flags = VERBOSE;
static int current_signal  = 0;

// Show usage for init
static cold noreturn void usage(int ret)
{
    printf("Usage: %s [runlevel]...\n"
           "    or %s --[opt]   ...\n"
           "  0           Poweroff\n"
           "  1, S, s     Switch to single user mode\n"
           "  2, 3, 4, 5  Switch to multi-user mode\n"
           "  6           Reboot\n"
           "  7           Halt\n"
           "  Q, q        Reload the current runlevel\n"
           "  --version   Show LeanInit's version number\n"
           "  --help      Show this usage information\n",
           __progname, __progname);
    exit(ret);
}

// Open the TTY
#if defined(FreeBSD) || defined(NetBSD)
static int open_tty(const char *tty_path)
{
    // Revoke access to the TTY if it is being used
    revoke(tty_path);
#else
static void open_tty(const char *tty_path)
{
#endif

    // Open the TTY
    int tty = open(tty_path, O_RDWR | O_NOCTTY);
    setsid();
    dup2(tty, STDOUT_FILENO);
    dup2(tty, STDERR_FILENO);
    dup2(tty, STDIN_FILENO); // This must be done last due to a bug on AMD GPUs
    ioctl(tty, TIOCSCTTY, 1);

#if defined(FreeBSD) || defined(NetBSD)
    // Return the file descriptor of the TTY
    return tty;
#endif
}

// Execute the given script
static int sh(char *script)
{
    pid_t child = fork();
    if (child == 0) {
        setsid();
        if ((flags & VERBOSE) == VERBOSE)
            return execve(script, (char *[]) { script, "verbose", NULL }, environ);
        else
            return execve(script, (char *[]) { script, "silent", NULL }, environ);
    } else if unlikely (child == -1)
        return -1;

    // Wait for the script to finish
    int status;
    waitpid(child, &status, 0);
    return WEXITSTATUS(status);
}

// Spawn a getty on the given TTY then return its PID
static pid_t spawn_getty(const char *cmd, const char *tty)
{
    pid_t getty = fork();
    if (getty == 0) {
        open_tty(tty);
        return execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
    }

    return getty;
}

// Return the accessible file path or NULL if neither are
static char *get_file_path(char *restrict primary, char *restrict fallback, int amode)
{
    if likely (access(primary, amode) == 0)
        return primary;
    else if likely (access(fallback, amode) == 0) // Using unlikely() here is a bad idea
        return fallback;
    else // Bottom priority
        return NULL;
}

// Single user mode (marked with cold as this is unlikely to be run during normal usage)
static cold void single(void)
{
    // Get the user's shell (the pathname can be rather long)
    char *shell = malloc(PATH_MAX);
    printf(CYAN "* " WHITE "Shell to use for single user (defaults to /bin/sh):" RESET " ");
    if(fgets(shell, PATH_MAX, stdin) != NULL) {
        shell[strcspn(shell, "\n")] = 0; // We don't want the newline
        if(access(shell, X_OK) != 0) {
            printf(PURPLE "* " YELLOW "Shell '%s' is invalid, defaulting to /bin/sh..." RESET "\n", shell);
            memcpy(shell, "/bin/sh", 8);
        }
    } else {
        // Ctrl + D
        printf(PURPLE "\n* " YELLOW "Defaulting to /bin/sh..." RESET "\n");
        memcpy(shell, "/bin/sh", 8);
    }

    // The actual shell
    pid_t sh = fork();
    if (sh == 0) {
        open_tty(DEFAULT_TTY);
        execve(shell, (char *[]) { shell, NULL }, environ);
    }

    // Free memory of the previous input
    free(shell);

    // When the shell is done, automatically reboot
    waitpid(sh, NULL, 0);
    kill(1, SIGINT);
}

// Execute rc(8) and getty(8) (multi-user)
static void multi(void)
{
    // Locate rc
    char *rc = get_file_path("/etc/leaninit/rc", "/etc/rc", X_OK);
    if unlikely (!rc) {
        printf(PURPLE "* " YELLOW
                      "Neither /etc/rc or /etc/leaninit/rc could be found, falling back to single user mode..." RESET
                      "\n");
        flags ^= SINGLE_USER;
        return single();
    }

    // Run rc
    if ((flags & VERBOSE) == VERBOSE)
        printf(CYAN "* " WHITE "Executing %s..." RESET "\n", rc);
    if unlikely (sh(rc) != 0) {
        printf(PURPLE "* " YELLOW "%s has failed, falling back to single user mode..." RESET "\n", rc);
        flags ^= SINGLE_USER;
        return single();
    }

    // Locate ttys(5)
    const char *ttys_file_path = get_file_path("/etc/leaninit/ttys", "/etc/ttys", R_OK);
    if unlikely (!ttys_file_path) {
        printf(RED "* Could not execute either /etc/leaninit/ttys or /etc/ttys" RESET "\n");
        return;
    }

    // Start a child process (for managing getty with plain wait(2))
    pid_t child = fork();
    if unlikely (child == -1) {
        printf(RED "* The child process for managing getty could not be created" RESET "\n");
        perror(RED "* fork()");
        return;
    } else if (child != 0)
        return;

    // Open the ttys file (max file size 8000 bytes with 60 entries)
    char *tofree, *data, *cmd;
    tofree = data = malloc(8001);
    unsigned char entry = 0;
    struct getty_t {
        const char *cmd;
        const char *tty;
        pid_t pid;
    } getty[60];
    FILE *ttys_file = fopen(ttys_file_path, "r");
    while (fgets(data, 8001, ttys_file) && entry != 60) {

        // Error checking
        if (strlen(data) < 2 || strchr(data, '#') != NULL)
            continue;
        cmd = strsep(&data, ":");
        if (strlen(cmd) < 2 || strlen(data) < 2)
            continue;

        // Spawn a getty
        entry++;
        getty[entry].pid = spawn_getty(cmd, data);
        getty[entry].cmd = cmd;
        getty[entry].tty = data;
    }

    // Close the ttys file and free the buffer from memory
    fclose(ttys_file);
    free(tofree);

    // Start the loop
    while (true) {
        int status;
        pid_t closed_pid = wait(&status);
        if unlikely (closed_pid == -1)
            return;

        // Match the closed PID to the getty in the index
        for (unsigned char e = 1; e <= entry; e++) {
            if (getty[e].pid != closed_pid)
                continue;

            // Do not spam the TTY if the getty failed
            if unlikely (WEXITSTATUS(status) != 0) {
#if defined(FreeBSD) || defined(NetBSD)
                open_tty(getty[e].tty);
#endif
                printf(RED "* The getty on %s has exited with a return status of %d" RESET "\n", getty[e].tty,
                       WEXITSTATUS(status));
                getty[e].pid = 0;
                break;
            }

            // Respawn the getty
            getty[e].pid = spawn_getty(getty[e].cmd, getty[e].tty);
            break;
        }
    }
}

// Run either single() for single user or multi() for multi user
static void *chlvl(unused void *notused)
{
    if unlikely ((flags & SINGLE_USER) == SINGLE_USER) // Most people boot into multi-user
        single();
    else
        multi();

    return NULL;
}

// This perpetual loop kills all zombie processes without blowing out CPU usage when there are none
static noreturn void *zloop(unused void *notused)
{
    while (true)
        if unlikely (wait(NULL) == -1)
            sleep(1);
}

// Set current_signal to the signal sent to PID 1
static void sighandle(int signal) { current_signal = signal; }

int main(int argc, char *argv[])
{
    // PID 1
    if (getpid() == 1) {

        // Open the console and login as root
#ifdef Linux
        open_tty(DEFAULT_TTY);
#else
        int tty = open_tty(DEFAULT_TTY);
#endif
        setenv("HOME", "/root", 1);
        setenv("LOGNAME", "root", 1);
        setenv("USER", "root", 1);

        // Over-optimized argument parsing
        const char *mode;
        argv += 1;
        while ((mode = *argv++)) {

            // Single user mode (accepts 'single' and '-s')
            if unlikely ((mode[0] == 's' && mode[1] == 'i' && mode[2] == 'n' && mode[3] == 'g' && mode[4] == 'l'
                          && mode[5] == 'e' && !mode[6])
                         || (mode[0] == '-' && mode[1] == 's'))
                flags |= SINGLE_USER;

            // Silent mode (accepts 'silent')
            else if (mode[0] == 's' && mode[1] == 'i' && mode[2] == 'l' && mode[3] == 'e' && mode[4] == 'n'
                     && mode[5] == 't' && !mode[6])
                flags &= ~(VERBOSE);

            // Run rc.banner (accepts 'banner')
            else if (mode[0] == 'b' && mode[1] == 'a' && mode[2] == 'n' && mode[3] == 'n' && mode[4] == 'e'
                     && mode[5] == 'r' && !mode[6])
                flags |= BANNER;
        }

        // Run rc.banner if the banner argument was passed to LeanInit
        if ((flags & BANNER) == BANNER) {
            char *rc_banner = get_file_path("/etc/leaninit/rc.banner", "/etc/rc.banner", X_OK);
            if likely (rc_banner != NULL)
                sh(rc_banner);
            else
                printf(RED "* Could not execute rc.banner(8)!" RESET "\n");
        }

        // Print the current platform LeanInit is running on and start rc(8) (must be done in this order)
        if ((flags & VERBOSE) == VERBOSE) {
            struct utsname uts;
            uname(&uts);
            printf(CYAN "* " WHITE "LeanInit " CYAN VERSION_NUMBER WHITE " is running on %s %s %s" RESET "\n",
                   uts.sysname, uts.release, uts.machine);
        }

        // Start both threads now
        pthread_t loop, runlvl;
        pthread_create(&runlvl, NULL, chlvl, NULL); // Create the runlevel in a separate thread
        pthread_create(&loop, NULL, zloop, NULL);   // Start the zombie killer

        // Handle all relevant signals
        struct sigaction actor;
        actor.sa_handler = sighandle; // Set the handler to sighandle()
        actor.sa_flags = 0;
        sigaction(SIGUSR1, &actor, NULL); // Halt
        sigaction(SIGUSR2, &actor, NULL); // Poweroff
        sigaction(SIGTERM, &actor, NULL); // Single-user
        sigaction(SIGILL, &actor, NULL);  // Multi-user
        sigaction(SIGHUP, &actor, NULL);  // Reload everything
        sigaction(SIGINT, &actor, NULL);  // Reboot

        // Signal handling loop
        int stored_signal;
        while (true) {

            // Wait for a signal, then store it to prevent race conditions
            pause();
            stored_signal = current_signal;

            // Cancel when the requested runlevel is already running
            if unlikely ((stored_signal == SIGILL && (flags & SINGLE_USER) != SINGLE_USER)
                         || (stored_signal == SIGTERM && (flags & SINGLE_USER) == SINGLE_USER))
                continue;

            /* Finish any I/O operations before executing rc.shutdown by calling sync(2),
               then join with the runlevel thread */
            sync();
            pthread_kill(runlvl, SIGKILL);
            pthread_join(runlvl, NULL);

            // Run rc.shutdown (which should handle sync), then kill all remaining processes with SIGKILL
            char *rc_shutdown = get_file_path("/etc/leaninit/rc.shutdown", "/etc/rc.shutdown", X_OK);
            if likely (rc_shutdown != NULL) {
                sh(rc_shutdown);
                if ((flags & VERBOSE) == VERBOSE)
                    printf(CYAN "* " WHITE "Killing all remaining processes that are still running..." RESET "\n");
            } else
                printf(RED "* Could not execute rc.shutdown(8), killing all processes unsafely..." RESET "\n");
            kill(-1, SIGKILL);

            // Handle the given signal properly
            switch (stored_signal) {

                // Halt
                case SIGUSR1:
                    return reboot(SYS_HALT);

                // Poweroff
                case SIGUSR2:
                    return reboot(SYS_POWEROFF);

                // Reboot
                case SIGINT:
                    return reboot(SYS_REBOOT);

                // Flip the bitmask value to set single user or multi-user
                case SIGTERM:
                case SIGILL:
                    flags ^= SINGLE_USER;
                    break;
            }

#if defined(FreeBSD) || defined(NetBSD)
            // Reopen the console on *BSD
            close(tty);
            tty = open_tty(DEFAULT_TTY);
#endif
            // Reload the runlevel
            pthread_create(&runlvl, NULL, chlvl, NULL);
        }
    }

    // Parse CLI arguments
    if unlikely (argc < 2) {
        usage(1);
        __builtin_unreachable();
    }

    // Handle --version and --help (micro-optimized for no good reason)
    if (argv[1][0] == '-' && argv[1][1] == '-') { // `--`
        if (argv[1][2] == 'v' && argv[1][3] == 'e' && argv[1][4] == 'r' && argv[1][5] == 's' && argv[1][6] == 'i'
            && argv[1][7] == 'o' && argv[1][8] == 'n' && !argv[1][9]) {
            // --version
            printf(CYAN "* " WHITE "LeanInit " CYAN VERSION_NUMBER RESET "\n");
            return 0;
        } else if (argv[1][2] == 'h' && argv[1][3] == 'e' && argv[1][4] == 'l' && argv[1][5] == 'p' && !argv[1][6]) {
            // --help
            usage(0);
            __builtin_unreachable();
        }
    }

    // Only root can send signals to LeanInit
    if unlikely (getuid() != 0) {
        printf(RED "* Permission denied!" RESET "\n");
        return 1;
    }

    // Switch runlevels by sending LeanInit the correct signal
    switch (*argv[1]) {

        // Poweroff
        case '0':
            return kill(1, SIGUSR2);

        // Single-user
        case '1':
        case 'S':
        case 's':
            return kill(1, SIGTERM);

        // Multi-user
        case '2':
        case '3':
        case '4':
        case '5':
            return kill(1, SIGILL);

        // Reload everything
        case 'Q':
        case 'q':
            return kill(1, SIGHUP);

        // Reboot
        case '6':
            return kill(1, SIGINT);

        // Halt
        case '7':
            return kill(1, SIGUSR1);

        // Fallback
        default:
            usage(1);
            __builtin_unreachable();
    }
}
