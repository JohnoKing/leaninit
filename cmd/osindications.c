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

int main(int argc, char *argv[]) {

	// Error checks
	if(getuid() != 0) {
		printf(RED "* Permission denied!" RESET "\n");
		return 1;
	} else if(access("/sys/firmware/efi/efivars/OsIndicationsSupported-8be4df61-93ca-11d2-aa0d-00e098032b8c", R_OK) != 0) {
		printf(RED "* This system does not support OsIndications!" RESET "\n");
		return 1;
	}

	// Long options and first_bytes variable
	size_t first_bytes = 4;
	struct option long_options[] = {
		{ "unset", no_argument, 0, 'u' },
		{ "help",  no_argument, 0, '?' },
	};

	// Parse options
	int args;
	while((args = getopt_long(argc, argv, "u?", long_options, NULL)) != -1) {
		switch(args) {

			// Display usage info
			case '?':
				printf("Usage: %s [-u?]\n", __progname);
				printf("  -u, --unset            Revert changes made by osindications\n");
				printf("  -?, --help             Show this usage information\n");
				return 1;

			// Unset OsIndications
			case 'u':
				first_bytes++;
				break;
		}
	}

	// Write efi_attr (0x0000000000000007)
	FILE *fd = fopen("/sys/firmware/efi/efivars/OsIndications-8be4df61-93ca-11d2-aa0d-00e098032b8c", "w+");
	unsigned long efi_attr = 0x0000000000000007;
	if(fwrite(&efi_attr, 1, first_bytes, fd) == 0) {
		fclose(fd);
		printf(RED "* Failed to write the changes to OsIndications!" RESET "\n");
		return 1;
	}

	// Write efi_boot (0x0000000000000001)
	if(first_bytes == 4) {
		unsigned long efi_boot = 0x0000000000000001;
		fwrite(&efi_boot, 1, 1, fd);
	}

	// Close the file and notify the user of the change
	fclose(fd);
	sync();
	if(first_bytes == 4) {
		printf(CYAN "* " WHITE "This system will now boot into the firmware's UI the next time it boots." RESET "\n");
		printf(CYAN "* " WHITE "Run `osindications --unset` to revert this change." RESET "\n");
	} else {
		printf(CYAN "* " WHITE "This system will not boot into the firmware's UI the next time it boots." RESET "\n");
		printf(CYAN "* " WHITE "Any prior changes made by osindications have been reverted." RESET "\n");
	}

	return 0;
}
