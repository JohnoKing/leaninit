/*
 * Copyright (c) 2017 Johnothan King. All rights reserved.
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
 * A minimal init system
 */

#include <sys/reboot.h>
#include <stdio.h>
#include <unistd.h>

// This function should never stop
static inline void rc(const char *m)
{
#ifdef OVERRIDE
    execl("/bin/sh", "/bin/sh", "/etc/rc", m, NULL);
#else
    execl("/bin/sh", "/bin/sh", "/etc/leaninit/rc", m, NULL);
#endif

    // In case the function stops
    sync();
}

// The main function
int main(int argc, char *argv[])
{
    // Execute the init script /etc/rc
    if(argc == 1) {
        rc("v");

    // SysV-like shutdown and reboot
    } else {
        // Synchronize the filesystems to prevent data corruption
        sync();

        // Determine what to do
        switch(*argv[1]) {
            case '0':
                reboot(RB_POWER_OFF);
                return 0;
            case '6':
                reboot(RB_AUTOBOOT);
                return 0;
            case 'q': // For quiet boot
                rc("q");
            case 's': // Defaults to quiet boot
                rc("q");
            case 'v': // Verbose boot (default)
                rc("v");
            default:
                printf("Argument invalid!\n\nUsage: init [mode] ...");
                return 1;
        }
    }
}
