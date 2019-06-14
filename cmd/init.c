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
static unsigned int single_user =  1;
static pid_t single_shell_pid   = -1;
static unsigned int verbose = 0;
static int current_signal   = 0;
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
static void sh(char *script)
{
    pid_t child = fork();
    if(child == 0) {
        setsid();
        char *sargv[] = { script, silent_flag, NULL };
        execve(script, sargv, environ);
    }

    waitpid(child, NULL, 0);
}

// Spawn a getty on the given tty then return its PID
static pid_t spawn_getty(const char *cmd, const char *tty)
{
    // Create the getty
    pid_t pid = fork();
    if(pid == 0) {
        open_tty(tty);
        execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
    }

    // Return the PID
    return pid;
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
        printf("\n");
    }

    // Fork the shell into a separate process
    single_shell_pid = fork();
    if(single_shell_pid == 0) {
        open_tty(DEFAULT_TTY);
        char *sargv[] = { shell, NULL };
        execve(shell, sargv, environ);
    }
}

// Execute rc(8) (multi-user)
static void multi(void)
{
    // Locate rc(8)
    char rc[17];
    if(access("/etc/leaninit/rc", X_OK) == 0)
        memcpy(rc, "/etc/leaninit/rc", 17);
    else if(access("/etc/rc", X_OK) == 0)
        memcpy(rc, "/etc/rc", 8);
    else {
        printf(PURPLE "* " YELLOW "Neither /etc/rc or /etc/leaninit/rc could be found, falling back to single user mode..." RESET "\n");
        single_user = 0;
        single();
        return;
    }

    // Run rc(8)
    if(verbose == 0) printf(CYAN "* " WHITE "Executing %s..." RESET "\n", rc);
    sh(rc);

    // Locate login(1)
    char login[16];
    if(access("/usr/bin/login", X_OK) == 0)
        memcpy(login, "/usr/bin/login", 15);
    else if(access("/bin/login", X_OK) == 0)
        memcpy(login, "/bin/login", 11);
    else {
        printf(RED "* Could not find login(1) (please symlink it to either /usr/bin/login or /bin/login and give it executable permissions)" RESET "\n");
        return;
    }

    // Locate ttys(5)
    char ttys_file_path[19];
    if(access("/etc/leaninit/ttys", R_OK) == 0)
        memcpy(ttys_file_path, "/etc/leaninit/ttys", 19);
    else if(access("/etc/ttys", R_OK) == 0)
        memcpy(ttys_file_path, "/etc/ttys", 10);
    else {
        printf(RED "* Could not find either /etc/leaninit/ttys or /etc/ttys" RESET "\n");
        return;
    }

    // Open ttys(5) and parse the data (max size 50000 bytes)
    char buffer[50001];
    char *data = buffer;
    FILE *ttys_file = fopen(ttys_file_path, "r");
    while(fgets(data, 50001, ttys_file)) {
        if(strlen(data) < 2 || strchr(data, '#') != NULL) continue;
        const char *getty_cmd = strsep(&data, ":");
        if(strlen(getty_cmd) < 2 || strlen (data) < 2) continue;
        spawn_getty(getty_cmd, data);
    }
    fclose(ttys_file);

    // TODO: Restart closed getty processes unless there was an error
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
    for(;;) wait(nullptr);
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

        // Login as root and set $PREVLEVEL to N
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
                if(access("/etc/leaninit/rc.shutdown", W_OK | X_OK) == 0)
                    sh("/etc/leaninit/rc.shutdown");
                else if(access("/etc/rc.shutdown", W_OK | X_OK) == 0)
                    sh("/etc/rc.shutdown");

                // Kill all remaining processes
                pthread_kill(runlvl, SIGKILL);
                pthread_join(runlvl, NULL);
                if(single_shell_pid != -1) {
                    kill(single_shell_pid, SIGKILL);
                    single_shell_pid = -1;
                }
                kill(-1, SIGCONT);  // For processes that have been sent SIGSTOP
                kill(-1, SIGTERM);

                // Give processes about seven seconds to comply with SIGTERM before sending SIGKILL
                struct timespec rest  = {0};
                rest.tv_nsec          = 100000000;
                unsigned int timer    = 0;
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
