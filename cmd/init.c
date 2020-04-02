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
 * leaninit - A fast init system
 */

#include <leaninit.h>

// Universal variables
static unsigned int single_user = 1;
static unsigned int verbose = 0;
static int current_signal = 0;

// Shows usage for init
static int usage(int ret)
{
    printf("Usage: %s [runlevel]...\n", __progname);
    printf("    or %s --[opt]   ...\n", __progname);
    printf("  0           Poweroff\n");
    printf("  1, S, s     Switch to single user mode\n");
    printf("  2, 3, 4, 5  Switch to multi-user mode\n");
    printf("  6           Reboot\n");
    printf("  7           Halt\n");
    printf("  Q, q        Reload the current runlevel\n");
    printf("  --version   Show LeanInit's version number\n");
    printf("  --help      Show this usage information\n");
    return ret;
}

// Open the tty
static int open_tty(const char *tty_path)
{
    // Revoke access to the tty if it is being used
#   ifdef FreeBSD
    revoke(tty_path);
#   endif

    // Open the tty
    int tty = open(tty_path, O_RDWR | O_NOCTTY);
    login_tty(tty);
    dup2(tty, STDOUT_FILENO);
    dup2(tty, STDERR_FILENO);
    dup2(tty, STDIN_FILENO);  // This must be done last
    ioctl(tty, TIOCSCTTY, 1);

    // Return the file descriptor of the tty
    return tty;
}

// Execute a script
static int sh(char *script)
{
    pid_t child = fork();
    if(child == 0) {
        setsid();
        if(verbose == 0) execve(script, (char*[]){ script, "verbose", NULL }, environ);
        else             execve(script, (char*[]){ script, "silent",  NULL }, environ);
        printf(RED "* Failed to run %s\n", script);
        perror("* execve()" RESET);
        return 1;
    } else if(child == -1) {
        printf(RED "* Failed to run %s\n", script);
        perror("* fork()" RESET);
        return 1;
    }

    // Wait for the script to finish
    int status;
    waitpid(child, &status, 0);
    return WEXITSTATUS(status);
}

// Spawn a getty on the given tty then return its PID
static pid_t spawn_getty(const char *cmd, const char *tty)
{
    pid_t getty = fork();
    if(getty == 0) {
        open_tty(tty);
        return execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
    }

    return getty;
}

// Return the accessible file path or NULL if neither are
static char *get_file_path(char *primary, char *fallback, int amode)
{
    if(access(primary, amode) == 0) return primary;
    else if(access(fallback, amode) == 0) return fallback;
    else return NULL;
}

// Single user mode
static void single(void)
{
    // Use a shell of the user's choice
    char buffer[101], shell[101];
    printf(CYAN "* " WHITE "Shell to use for single user (defaults to /bin/sh):" RESET " ");
    (void) fgets(buffer, 101, stdin);
    sscanf(buffer, "%s", shell);

    // If the given shell is invalid, use /bin/sh instead
    if(access(shell, X_OK) != 0) {
        printf(PURPLE "\n* " YELLOW "Defaulting to /bin/sh..." RESET "\n");
        memcpy(shell, "/bin/sh", 8);
    }

    // Fork the shell as the child of a managing child process (trying to use only one child process causes serious bugs)
    pid_t child = fork();
    if(child == 0) {

        // Actual shell
        pid_t sh = fork();
        if(sh == 0) {
            open_tty(DEFAULT_TTY);
            execve(shell, (char*[]){ shell, NULL }, environ);
            printf(RED "* Failed to run %s\n", shell);
            perror("* execve()" RESET);
            kill(1, SIGFPE);
        } else if(sh == -1) {
            printf(RED "* Failed to run %s\n", shell);
            perror("* fork()" RESET);
            kill(1, SIGFPE);
        }

        // When the shell is done, automatically reboot
        waitpid(sh, NULL, 0);
        kill(1, SIGINT);

    // Extra error handling
    } else if(child == -1) {
        printf(RED "* Failed to run %s\n", shell);
        perror("* fork()" RESET);
        kill(1, SIGFPE);
    }
}

// Execute rc(8) and getty(8) (multi-user)
static void multi(void)
{
    // Locate rc
    char *rc = get_file_path("/etc/leaninit/rc", "/etc/rc", X_OK);
    if(rc == NULL) {
        printf(PURPLE "* " YELLOW "Neither /etc/rc or /etc/leaninit/rc could be found, falling back to single user mode..." RESET "\n");
        single_user = 0;
        return single();
    }

    // Run rc
    if(verbose == 0) printf(CYAN "* " WHITE "Executing %s..." RESET "\n", rc);
    if(sh(rc) != 0) {
        printf(PURPLE "* " YELLOW "%s has failed, falling back to single user mode..." RESET "\n", rc);
        single_user = 0;
        return single();
    }

    // Locate ttys(5)
    const char *ttys_file_path = get_file_path("/etc/leaninit/ttys", "/etc/ttys", R_OK);
    if(ttys_file_path == NULL) {
        printf(RED "* Could not find either /etc/leaninit/ttys or /etc/ttys" RESET "\n");
        return;
    }

    // Start a child process (for managing getty with plain wait(2))
    pid_t child = fork();
    if(child == -1) {
        printf(RED "* The child process for managing getty could not be created" RESET "\n");
        perror(RED "* fork()");
        return;
    } else if(child != 0) return;

    // Open the ttys file (max file size 8000 bytes with 60 entries)
    char buffer[8001];
    char *data = buffer;
    unsigned int entry = 0;
    struct getty_t {
        const char *cmd;
        const char *tty;
        pid_t pid;
    } getty[60];
    FILE *ttys_file = fopen(ttys_file_path, "r");
    while(fgets(data, 8001, ttys_file) && entry != 60) {

        // Error checking
        if(strlen(data) < 2 || strchr(data, '#') != NULL) continue;
        const char *cmd = strsep(&data, ":");
        if(strlen(cmd) < 2 || strlen(data) < 2) continue;

        // Spawn a getty
        entry++;
        getty[entry].pid = spawn_getty(cmd, data);
        getty[entry].cmd = cmd;
        getty[entry].tty = data;
    }

    // Close the ttys file
    fclose(ttys_file);

    // Start the loop
    for(;;) {
        int status;
        pid_t closed_pid = wait(&status);
        if(closed_pid == -1) return;

        // Match the closed PID to the getty in the index
        for(unsigned int e = 1; e <= entry; e++) {
            if(getty[e].pid != closed_pid) continue;

            // Do not spam the tty if the getty failed
            if(WEXITSTATUS(status) != 0) {
#               ifdef FreeBSD
                open_tty(getty[e].tty);
#               endif
                printf(RED "* The getty on %s has exited with a return status of %d" RESET "\n", getty[e].tty, WEXITSTATUS(status));
                getty[e].pid = 0;
                break;
            }

            // Respawn the getty
            getty[e].pid = spawn_getty(getty[e].cmd, getty[e].tty);
            break;
        }
    }
}

// Run either single() or multi() depending on the value of single_user
static void *chlvl(void *nullptr)
{
    if(single_user == 0) single();
    else multi();

    return nullptr;
}

// This perpetual loop kills all zombie processes without blowing out CPU usage when there are none
static noreturn *zloop(void *nullptr)
{
    for(;;) if(wait(nullptr) == -1) sleep(1);
}

// Set current_signal to the signal sent to PID 1
static void sighandle(int signal)
{
    current_signal = signal;
}

int main(int argc, char *argv[])
{
    // PID 1
    if(getpid() == 1) {

        // Open the console and login as root
        int tty = open_tty(DEFAULT_TTY);
        setenv("HOME",   "/root", 1);
        setenv("LOGNAME", "root", 1);
        setenv("USER",    "root", 1);

        // Single user (argv = single/-s), silent mode (argv = silent) and rc.banner(8) support
        unsigned int banner = 0;
        --argc;
        while(0 < argc) {
            if(strcmp(argv[argc], "single") == 0 || strcmp(argv[argc], "-s") == 0) single_user = 0;
            else if(strcmp(argv[argc], "silent") == 0) verbose = 1;
            else if(strcmp(argv[argc], "banner") == 0) banner  = 1;
            --argc;
        }

        // Start the zombie killer thread
        pthread_t loop, runlvl;
        pthread_create(&loop, NULL, zloop, NULL);

        // Run rc.banner if the banner argument was passed to LeanInit
        if(banner != 0) {
            char *rc_banner = get_file_path("/etc/leaninit/rc.banner", "/etc/rc.banner", X_OK);
            if(rc_banner != NULL)
                sh(rc_banner);
            else
                printf(RED "* Could not find rc.banner(8)!" RESET "\n");
        }

        // Print the current platform LeanInit is running on and start rc(8) (must be done in this order)
        if(verbose == 0) {
            struct utsname uts;
            uname(&uts);
            printf(CYAN "* " WHITE "LeanInit " CYAN VERSION_NUMBER WHITE " is running on %s %s %s" RESET "\n", uts.sysname, uts.release, uts.machine);
        }
        pthread_create(&runlvl, NULL, chlvl, NULL);

        // Handle relevant signals while ignoring others
        struct sigaction actor;
        memset(&actor, 0, sizeof(actor)); // Without this sigaction is ineffective
        actor.sa_handler = sighandle;     // Set the handler to sighandle()
        sigaction(SIGUSR1, &actor, NULL); // Halt
        sigaction(SIGUSR2, &actor, NULL); // Poweroff
        sigaction(SIGFPE,  &actor, NULL); // Poweroff (delayed)
        sigaction(SIGTERM, &actor, NULL); // Single-user
        sigaction(SIGILL,  &actor, NULL); // Multi-user
        sigaction(SIGHUP,  &actor, NULL); // Reload everything
        sigaction(SIGINT,  &actor, NULL); // Reboot

        // Signal handling loop
        for(;;) {

            // Wait for a signal, then store it in stored_signal to prevent race conditions
            pause();
            int stored_signal = current_signal;

            // Cancel when the requested runlevel is already running
            if((stored_signal == SIGILL && single_user != 0) || (stored_signal == SIGTERM && single_user == 0))
                continue;

            // Finish any I/O operations before executing rc.shutdown by calling sync(2), then join with the runlevel thread
            sync();
            pthread_kill(runlvl, SIGKILL);
            pthread_join(runlvl, NULL);

            // Run rc.shutdown (which should handle sync), then kill all remaining processes with SIGKILL
            char *rc_shutdown = get_file_path("/etc/leaninit/rc.shutdown", "/etc/rc.shutdown", X_OK);
            if(rc_shutdown != NULL) sh(rc_shutdown);
            if(verbose == 0) printf(CYAN "* " WHITE "Killing all remaining processes that are still running..." RESET "\n");
            kill(-1, SIGKILL);

            // Handle the given signal properly
            switch(stored_signal) {

                // Halt
                case SIGUSR1:
                    return reboot(SYS_HALT);

                // Poweroff
                case SIGFPE: // Delay
#                   ifdef FreeBSD
                    close(tty);
                    open_tty(DEFAULT_TTY);
#                   endif
                    printf(CYAN "* " WHITE "Delaying shutdown for three seconds..." RESET "\n");
                    sleep(3);
                /*FALLTHRU*/
                case SIGUSR2:
                    return reboot(SYS_POWEROFF);

                // Reboot
                case SIGINT:
                    return reboot(RB_AUTOBOOT);

                // Switch to single user
                case SIGTERM:
                    single_user = 0;
                    break;

                // Switch to multi-user
                case SIGILL:
                    single_user = 1;
                    break;
            }

            // Reopen the console and reload the runlevel
            close(tty);
            tty = open_tty(DEFAULT_TTY);
            pthread_create(&runlvl, NULL, chlvl, NULL);
        }
    }

    // Parse CLI arguments
    if(argc < 2)
        return usage(1);

    // Check for --version and --help
    if(strcmp(argv[1], "--version") == 0) {
        printf(CYAN "* " WHITE "LeanInit " CYAN VERSION_NUMBER RESET "\n");
        return 0;
    } else if(strcmp(argv[1], "--help") == 0)
        return usage(0);

    // Only root can send signals to LeanInit
    if(getuid() != 0) {
        printf(RED "* Permission denied!" RESET "\n");
        return 1;
    }

    // Switch runlevels by sending LeanInit the correct signal
    switch(*argv[1]) {

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
            return usage(1);
    }
}
