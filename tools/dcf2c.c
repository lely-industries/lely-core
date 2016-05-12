/*!\file
 * This file contains the CANopen EDS/DCF to C conversion tool.
 *
 * \copyright 2016 Lely Industries N.V.
 *
 * \author J. S. Seldenthuis <jseldenthuis@lely.com>
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

#include <lely/util/diag.h>
#include <lely/co/dcf.h>
#include <lely/co/sdev.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMD_USAGE \
	"Arguments: [options] <filename> <name>\n" \
	"Options:\n" \
	"  -h, --help            Display this information\n" \
	"  --no-strings          Do not include optional strings in the output\n" \
	"  -o <file>, --output=<file>\n" \
	"                        Write the output to <file> instead of stdout\n"

#define FLAG_NO_STRINGS	0x01

void cmd_diag_handler(void *handle, enum diag_severity severity, errc_t errc,
		const char *format, va_list ap);

int
main(int argc, char *argv[])
{
	const char *cmd = cmdname(argv[0]);
	diag_set_handler(&cmd_diag_handler, (void *)cmd);

	int flags = 0;
	const char *ifname = NULL;
	const char *ofname = NULL;
	const char *name = NULL;
	for (int i = 1; i < argc; i++) {
		char *opt = argv[i];
		if (*opt == '-') {
			opt++;
			if (*opt == '-') {
				opt++;
				if (!strcmp(opt, "help")) {
					diag(DIAG_INFO, 0, CMD_USAGE);
					goto error_arg;
				} else if (!strcmp(opt, "no-strings")) {
					flags |= FLAG_NO_STRINGS;
				} else if (!strncmp(opt, "output=", 7)) {
					ofname = opt + 7;
				} else {
					diag(DIAG_ERROR, 0, "invalid option '--%s'",
							opt);
					goto error_arg;
				}
			} else {
				for (; *opt; opt++) {
					switch (*opt) {
					case 'h':
						diag(DIAG_INFO, 0, CMD_USAGE);
						goto error_arg;
					case 'o':
						if (__unlikely(++i >= argc)) {
							diag(DIAG_ERROR, 0, "option '-%c' requires an argument",
									*opt);
							goto error_arg;
						}
						ofname = argv[i];
						break;
					default:
						diag(DIAG_ERROR, 0, "invalid option '-%c'",
								*opt);
						goto error_arg;
					}
				}
			}
		} else {
			if (!ifname)
				ifname = opt;
			else if (!name)
				name = opt;
		}
	}

	if (__unlikely(!ifname)) {
		diag(DIAG_ERROR, 0, "no filename specified");
		goto error_arg;
	}

	if (__unlikely(!name)) {
		diag(DIAG_ERROR, 0, "no variable name specified");
		goto error_arg;
	}

	co_dev_t *dev = co_dev_create_from_dcf_file(ifname);
	if (__unlikely(!dev))
		goto errror_create_dev;

	size_t n = snprintf_c99_sdev(NULL, 0, dev);
	char *s = malloc(n + 1);
	if (__unlikely(n && !s)) {
		diag(DIAG_ERROR, get_errc(), "unable to allocate string");
		goto error_malloc_s;
	}
	snprintf_c99_sdev(s, n + 1, dev);

	FILE *stream = stdout;
	if (ofname) {
		stream = fopen(ofname, "w");
		if (__unlikely(!stream)) {
			diag(DIAG_ERROR, get_errc(), "unable to open %s for writing",
					ofname);
			goto error_fopen;
		}
	}

	fprintf(stream, "#include <lely/co/sdev.h>\n\n"
			"#define CO_SDEV_STRING(s)\t%s\n\n"
			"const struct co_sdev %s = %s;\n\n",
			(flags & FLAG_NO_STRINGS) ? "NULL" : "s", name, s);

	if (ofname)
		fclose(stream);
	free(s);
	co_dev_destroy(dev);

	return EXIT_SUCCESS;

error_fopen:
	free(s);
error_malloc_s:
	co_dev_destroy(dev);
errror_create_dev:
error_arg:
	return EXIT_FAILURE;
}

void
cmd_diag_handler(void *handle, enum diag_severity severity, errc_t errc,
		const char *format, va_list ap)
{
	if (handle)
		fprintf(stderr, "%s: ", (const char *)handle);
	default_diag_handler(handle, severity, errc, format, ap);
}

