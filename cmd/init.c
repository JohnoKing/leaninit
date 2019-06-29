/*
 * Copyright (c) 2017-2019 Johnothan King. All rights reserved.
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
static int current_signal = 0;
static vint_t single_user = 1;
static vint_t verbose =  0;
static pid_t su_shell = -1;
static char silent_flag[2];

// Shows usage for init
static int usage(int ret)
{
    printf("Usage: %s [runlevel] ...\n", __progname);
    printf("    or %s --[opt]    ...\n", __progname);
    printf("  0           Poweroff\n");
    printf("  1, S, s     Switch to single user mode\n");
    printf("  2, 3, 4, 5  Switch to multi-user mode\n");
    printf("  6           Reboot\n");
    printf("  7           Halt\n");
#   ifdef Linux
    printf("  8           Hibernate\n");
#   endif
    printf("  Q, q        Reload the current runlevel\n");
    printf("  --version   Shows LeanInit's version number\n");
    printf("  --help      Shows this usage information\n");
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
    dup2(tty, STDIN_FILENO);
    dup2(tty, STDOUT_FILENO);
    dup2(tty, STDERR_FILENO);
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
        char *sargv[] = { script, silent_flag, NULL };
        execve(script, sargv, environ);
        printf(RED "* Failed to run %s" RESET "\n", script);
        perror(RED "* execve()" RESET);
        return 1;
    } else if(child == -1) {
        printf(RED "* Failed to run %s" RESET "\n", script);
        perror(RED "* fork()" RESET);
        return 1;
    }

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
static char *write_file_path(char *primary, char *fallback, int amode)
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
    fgets(buffer, 101, stdin);
    sscanf(buffer, "%s", shell);

    // If the given shell is invalid, use /bin/sh instead
    if(access(shell, X_OK) != 0) {
        printf(PURPLE "\n* " YELLOW "Defaulting to /bin/sh..." RESET "\n");
        memcpy(shell, "/bin/sh", 8);
    }

    // Fork the shell into a separate process
    su_shell = fork();
    if(su_shell == 0) {
        open_tty(DEFAULT_TTY);
        char *sargv[] = { shell, NULL };
        execve(shell, sargv, environ);
        printf(RED "* Failed to run %s" RESET "\n", shell);
        perror(RED "* execve()" RESET);
        printf(RED "* This system will now shutdown in three seconds..." RESET "\n");
        kill(1, SIGFPE);
    } else if(su_shell == -1) {
        printf(RED "* Failed to run %s" RESET "\n", shell);
        perror(RED "* fork()" RESET);
        printf(RED "* This system will now shutdown in three seconds..." RESET "\n");
        kill(1, SIGFPE);
    }
}

// Execute rc(8) and getty(8) (multi-user)
static void multi(void)
{
    // Locate rc
    char *rc = write_file_path("/etc/leaninit/rc", "/etc/rc", X_OK);
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

    // Locate login(1)
    const char *login_path = write_file_path("/usr/bin/login", "/bin/login", X_OK);
    if(login_path == NULL) {
        printf(RED "* Could not find login(1) (please symlink it to either /usr/bin/login or /bin/login and give it executable permissions)" RESET "\n");
        return;
    }

    // Locate ttys(5)
    const char *ttys_file_path = write_file_path("/etc/leaninit/ttys", "/etc/ttys", R_OK);
    if(ttys_file_path == NULL) {
        printf(RED "* Could not find either /etc/leaninit/ttys or /etc/ttys" RESET "\n");
        return;
    }


    // Start a child process (for managing getty with plain wait(2))
    if(fork() != 0) return;

    // Open ttys (max file size 40000 bytes with 90 entries)
    FILE *ttys_file = fopen(ttys_file_path, "r");
    char buffer[40001];
    char *data = buffer;
    vint_t entry = 0;
    struct getty_t {
        const char *cmd;
        const char *tty;
        pid_t pid;
    } getty[90];
    while(fgets(data, 40001, ttys_file) && entry != 90) {

        // Error checking
        if(strlen(data) < 2 || strchr(data, '#') != NULL) continue;
        const char *cmd = strsep(&data, ":");
        if(strlen(cmd) < 2 || strlen (data) < 2) continue;

        // Spawn getty
        entry++;
        getty[entry].pid = spawn_getty(cmd, data);
        getty[entry].cmd = cmd;
        getty[entry].tty = data;
    }

    // Close ttys
    fclose(ttys_file);

    // Start the loop
    for(;;) {
        int status;
        pid_t closed_pid = wait(&status);
        if(closed_pid == -1) return;

        // Match the closed pid to the getty in the index
        for(vint_t e = 1; e <= entry; e++) {
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

// Run either single() or multi() depending on the runlevel
static void *chlvl(void *nullptr)
{
    // Run single user if single_user is equal to 0, otherwise run multi-user
    if(single_user == 0) single();
    else multi();

    return nullptr;
}

// This perpetual loop kills all zombie processes
__attribute((noreturn)) static void *zloop(void *nullptr)
{
    for(;;) if(wait(nullptr) == -1) sleep(1);
}

// Set current_signal to the signal sent to PID 1
static void sighandle(int signal)
{
    current_signal = signal;
}

// The main function
int main(int argc, char *argv[])
{
    // PID 1
    if(getpid() == 1) {

        // Login as root
        setenv("HOME",   "/root", 1);
        setenv("LOGNAME", "root", 1);
        setenv("USER",    "root", 1);

        // Single user support (argv = -s)
        int args;
        while((args = getopt(argc, argv, "s")) != -1) {
            switch(args) {
                case 's':
                    single_user = 0;
                    break;
            }
        }

        // Single user (argv = single) and silent mode (argv = silent) support
        if(single_user != 0) {
            args = argc - 1;
            while(0 < args) {

                // Single user mode
                if(strcmp(argv[args], "single") == 0) single_user = 0;

                // Silent mode
                else if(strcmp(argv[args], "silent") == 0) {
                    memcpy(silent_flag, "s", 2);
                    verbose = 1;
                }

                --args;
            }
        }

        // Open the console
        int tty = open_tty(DEFAULT_TTY);

        // Print the current platform LeanInit is running on
        struct utsname uts;
        uname(&uts);
        if(verbose == 0) printf(CYAN "* " WHITE "LeanInit " CYAN VERSION_NUMBER WHITE " is running on %s %s %s" RESET "\n", uts.sysname, uts.release, uts.machine);

        // Start zloop() and chlvl() in separate threads
        pthread_t loop, runlvl;
        pthread_create(&loop,   NULL, zloop, NULL);
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
        sigaction(SIGHUP,  &actor, NULL); // Reloads everything
        sigaction(SIGINT,  &actor, NULL); // Reboot

        // Signal handling loop
        for(;;) {
            // Wait for a signal to be sent to init
            pause();

            // Store the received signal in recv_signal to prevent race conditions
            int recv_signal = current_signal;

            // Cancel when the runlevel is already the currently running one
            if(recv_signal == SIGILL && single_user != 0)
                printf("\n" PURPLE "* " YELLOW "LeanInit is already in multi-user mode..."  RESET "\n");

            else if(recv_signal == SIGTERM && single_user == 0)
                printf("\n" PURPLE "* " YELLOW "LeanInit is already in single user mode..." RESET "\n");

            // Switch the current runlevel
            else {
                // Synchronize all file systems (Pass 1)
                sync();

                // Run rc.shutdown (multi-user)
                char *rc_shutdown = write_file_path("/etc/leaninit/rc.shutdown", "/etc/rc.shutdown", X_OK);
                if(rc_shutdown != NULL) sh(rc_shutdown);

                // Kill all remaining processes
                pthread_kill(runlvl, SIGKILL);
                pthread_join(runlvl, NULL);
                if(su_shell != -1) {
                    kill(su_shell, SIGKILL);
                    su_shell = -1;
                }
                kill(-1, SIGCONT);  // For processes that have been sent SIGSTOP
                kill(-1, SIGTERM);

                // Give processes about seven seconds to comply with SIGTERM before sending SIGKILL
                struct timespec rest  = {0};
                rest.tv_nsec          = 100000000;
                vint_t timer = 0;
                while(waitpid(-1, NULL, WNOHANG) != -1 && timer < 70) {
                    nanosleep(&rest, NULL);
                    timer++;
                }
                kill(-1, SIGKILL);

                // Synchronize file systems again (Pass 2)
                sync();

                // Handle the given signal properly
                switch(recv_signal) {

                    // Halt
                    case SIGUSR1:
                        return reboot(SYS_HALT);

                    // Poweroff
                    case SIGFPE:  // Delay
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

            // Reset the signal
            current_signal = 0;
        }
    }

    // Emulate some SysV-like behavior when re-executed
    if(argc < 2)
        return usage(1);

    // Show the version number when called with --version
    if(strcmp(argv[1], "--version") == 0) {
        printf(CYAN "* " WHITE "LeanInit " CYAN VERSION_NUMBER RESET "\n");
        return 0;
    } else if(strcmp(argv[1], "--help") == 0)
        return usage(0);

    // Prevent everyone except root from running any of the following code
    if(getuid() != 0) {
        printf(RED "* Permission denied!" RESET "\n");
        return 1;
    }

    // Switches runlevels by sending PID 1 the correct signal
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

        // Hibernate
#       ifdef Linux
        case '8':
            return reboot(RB_SW_SUSPEND);
#       endif

        // Fallback
        default:
            return usage(1);
    }
}
