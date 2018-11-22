/**@file
 * This header file is part of the C11 and POSIX compatibility library; it
 * includes `<unistd.h>`, if it exists, and defines any missing functionality.
 *
 * @copyright 2013-2018 Lely Industries N.V.
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

#ifndef LELY_LIBC_UNISTD_H_
#define LELY_LIBC_UNISTD_H_

#include <lely/features.h>

#ifndef LELY_HAVE_UNISTD_H
#if defined(_POSIX_C_SOURCE) || defined(__MINGW32__) || defined(__NEWLIB__)
#define LELY_HAVE_UNISTD_H 1
#endif
#endif

#if LELY_HAVE_UNISTD_H
#include <unistd.h>
#else // !LELY_HAVE_UNISTD_H

#ifdef __cplusplus
extern "C" {
#endif

/// A pointer to the argument of the current option.
char *optarg;

/// The index of the next argument to be parsed by getopt().
int optind;

/**
 * A flag indicating whether a diagnostic message should be printed if an
 * unknown option character or missing argument is detected by getopt(). The
 * default value is 1.
 */
int opterr;

/// The last option character to cause an error.
int optopt;

/**
 * Parses options passed as arguments to `main()`.
 *
 * @param argc      the argument count as passed to `main()`.
 * @param argv      the argument array as passed to `main()`.
 * @param optstring a pointer to a string of recognized option characters; if a
 *                  character is followed by a colon, the option takes an
 *                  argument. If the first character is a colon, this function
 *                  returns ':' instead of '?' when a missing argument is
 *                  detected and does not print a diagnostic message (regardless
 *                  of the value of #opterr).
 *
 * @returns the next option character, '?' if an unknown option character or
 * missing argument is detected, or -1 if all the argument is not an option or
 * all options are parsed.
 */
int getopt(int argc, char *const argv[], const char *optstring);

/**
 * Sleeps until the specified number of realtime seconds has elapsed or the
 * calling thread is interrupted.
 *
 * @returns 0 if the requested time has elapsed, or the remaining number of
 * seconds if not.
 *
 * @see nanosleep()
 */
unsigned sleep(unsigned seconds);

#ifdef __cplusplus
}
#endif

#endif // !LELY_HAVE_UNISTD_H

#endif // !LELY_LIBC_UNISTD_H_
