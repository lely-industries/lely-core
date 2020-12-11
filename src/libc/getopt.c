/**@file
 * This file is part of the C11 and POSIX compatibility library.
 *
 * @see lely/libc/unistd.h
 *
 * @copyright 2016-2018 Lely Industries N.V.
 *
 * @author J. S. Seldenthuis <jseldenthuis@lely.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "libc.h"
#include <lely/libc/unistd.h>

#if !LELY_HAVE_UNISTD_H

#include <assert.h>
#include <stddef.h>
#if !LELY_NO_STDIO
#include <stdio.h>
#endif

char *optarg;
int optind = 1;
int opterr = 1;
int optopt;

/**
 * The offset (in characters) of the next option with respect to the beginning
 * of the current option argument.
 */
static int optoff;

int
getopt(int argc, char *const argv[], const char *optstring)
{
	assert(argv);
	assert(optstring);

	optarg = NULL;

	// Check whether any arguments remain.
	if (optind >= argc || !argv[optind])
		return -1;
	// Continue with the next option in the current argument.
	char *cp = argv[optind] + optoff;
	// If this is a new argument, check if it is an option argument.
	if (!optoff) {
		// An option argument begins with '-'.
		if (*cp++ != '-' || !*cp)
			return -1;
		// A double dash ("--") denotes the end of option arguments.
		if (*cp == '-' && !cp[1]) {
			optind++;
			return -1;
		}
		optoff = 1;
	}
	int c = *cp++;
	// Update the index and offset of the next option.
	if (*cp) {
		optoff++;
	} else {
		optind++;
		optoff = 0;
	}

	// Check if the option character occurs in `optstring`.
	const char *op = optstring;
	while (*op && (*op == ':' || *op == '?' || *op != c))
		op++;
	if (!*op) {
		optopt = c;
#if !LELY_NO_STDIO
		if (opterr && *optstring != ':')
			fprintf(stderr, "%s: illegal option -- %c\n", argv[0],
					optopt);
#endif
		return '?';
	}

	// If the option does not take an argument, we are done.
	if (*++op != ':')
		return c;

	optind++;
	optoff = 0;
	if (*cp) {
		// If any characters remain in the current argument, they form
		// the argument for the option...
		optarg = cp;
	} else {
		// ... otherwise, the next argument is used, if it exists.
		if (optind > argc) {
			optopt = c;
#if !LELY_NO_STDIO
			if (opterr && *optstring != ':')
				fprintf(stderr,
						"%s: option requires an "
						"argument -- %c\n",
						argv[0], optopt);
#endif
			return *optstring == ':' ? ':' : '?';
		}
		optarg = argv[optind - 1];
	}
	return c;
}

#endif // !LELY_HAVE_UNISTD_H
