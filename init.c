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

// Execute the init script
static inline void rc(const char *mode) {

    // Execute either /etc/rc or /etc/leaninit/rc
#ifdef OVERRIDE
    execl("/bin/sh", "/bin/sh", "/etc/rc", mode, NULL);
#else
    execl("/bin/sh", "/bin/sh", "/etc/leaninit/rc", mode, NULL);
#endif

    // This is done in case this function stops to prevent data corruption
    sync();
}

// The main function
int main(int argc, char *argv[])
{
    // Prevent anyone but root from running this
    if(getuid() != 0) {
        printf("LeanInit must be run as root!\n");
        return 1;
    }

    // Defaults to verbose boot
    if(argc == 1) {
        rc("v");

    // Determine what to do
    } else {
        switch(*argv[1]) {

            // Poweroff
            case '0':
                sync();
                reboot(RB_POWER_OFF);

            // Reboot
            case '6':
                sync();
                reboot(RB_AUTOBOOT);

            // Quiet boot, splash boot currently defaults to quiet boot
            case 'q':
            case 's':
                rc("q");

            // Verbose boot (default)
            case 'v':
                rc("v");

            // Fallback
            default:
                printf("Argument invalid!\nUsage: init [mode] ...\n");
                return 1;
        }
    }
}
