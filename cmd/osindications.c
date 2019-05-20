/*
 * Copyright (c) 2019 Johnothan King. All rights reserved.
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
 * osindications - This program sets OsIndications for booting into firmware setup.
 */

#include "inc.h"

int main(void) {

	// Error checks
	if(getuid() != 0) {
		printf(RED "* Permission denied!" RESET "\n");
		return 1;
	else if(access("/sys/firmware/efi/efivars/OsIndicationsSupported-8be4df61-93ca-11d2-aa0d-00e098032b8c", W_OK) != 0) {
		printf(RED "* This system does not support OsIndications!" RESET "\n");
		return 1;
	}

	// OsIndications variables
	unsigned long efi_attr = 0x0000000000000007;
	unsigned long efi_boot = 0x0000000000000001;

	// Write to OsIndications
	FILE *fd = fopen("/sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c", "w+");
	fwrite(&efi_attr, 1, 4, fd);
	fwrite(&efi_boot, 1, 1, fd);
	fclose(fd);
	sync();

	// Notify the user of the change
	printf(CYAN "* " WHITE "This system will now boot into the firmware's UI the next time it boots." RESET "\n");
	return 0;
}
