/**@file
 * This file contains the CANopen EDS/DCF to C conversion tool.
 *
 * @copyright 2017-2020 Lely Industries N.V.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <lely/co/dcf.h>
#include <lely/co/sdev.h>
#include <lely/libc/stdio.h>
#include <lely/libc/unistd.h>
#include <lely/util/diag.h>

#include <stdlib.h>
#include <string.h>

// clang-format off
#define HELP \
	"Arguments: [options...] filename <variable name>\n" \
	"Options:\n" \
	"  -h, --help            Display this information\n" \
	"  --no-strings          Do not include optional strings in the output\n" \
	"  -o <file>, --output=<file>\n" \
	"                        Write the output to <file> instead of stdout"
// clang-format on

#define FLAG_HELP 0x01
#define FLAG_NO_STRINGS 0x02

int
main(int argc, char *argv[])
{
	argv[0] = (char *)cmdname(argv[0]);
	diag_set_handler(&cmd_diag_handler, argv[0]);

	int flags = 0;
	const char *ifname = NULL;
	const char *ofname = NULL;
	const char *name = NULL;

	opterr = 0;
	optind = 1;
	int optpos = 0;
	while (optind < argc) {
		char *arg = argv[optind];
		if (*arg != '-') {
			optind++;
			switch (optpos++) {
			case 0: ifname = arg; break;
			case 1: name = arg; break;
			default:
				diag(DIAG_ERROR, 0, "extra argument %s", arg);
				break;
			}
		} else if (*++arg == '-') {
			optind++;
			if (!*++arg)
				break;
			if (!strcmp(arg, "help")) {
				flags |= FLAG_HELP;
			} else if (!strcmp(arg, "no-strings")) {
				flags |= FLAG_NO_STRINGS;
			} else if (!strncmp(arg, "output=", 7)) {
				ofname = arg + 7;
			} else {
				diag(DIAG_ERROR, 0, "illegal option -- %s",
						arg);
			}
		} else {
			int c = getopt(argc, argv, ":ho:");
			if (c == -1)
				break;
			switch (c) {
			case ':':
				diag(DIAG_ERROR, 0,
						"option requires an argument -- %c",
						optopt);
				break;
			case '?':
				diag(DIAG_ERROR, 0, "illegal option -- %c",
						optopt);
				break;
			case 'h': flags |= FLAG_HELP; break;
			case 'o': ofname = optarg; break;
			}
		}
	}
	for (char *arg = argv[optind]; optind < argc; arg = argv[++optind]) {
		switch (optpos++) {
		case 0: ifname = arg; break;
		case 1: name = arg; break;
		default: diag(DIAG_ERROR, 0, "extra argument %s", arg); break;
		}
	}

	if (flags & FLAG_HELP) {
		diag(DIAG_INFO, 0, "%s", HELP);
		return EXIT_SUCCESS;
	}

	if (optpos < 1 || !ifname) {
		diag(DIAG_ERROR, 0, "no filename specified");
		goto error_arg;
	}

	if (optpos < 2 || !name) {
		diag(DIAG_ERROR, 0, "no variable name specified");
		goto error_arg;
	}

	co_dev_t *dev = NULL;
	if (!strcmp(ifname, "-")) {
		char *line = NULL;
		size_t n = 0;
		if (getdelim(&line, &n, '\0', stdin) == -1 && ferror(stdin)) {
			free(line);
			diag(DIAG_ERROR, get_errc(),
					"unable to read from standard input");
			goto error_getdelim;
		}
		struct floc at = { "<stdin>", 1, 1 };
		dev = co_dev_create_from_dcf_text(line, line + n, &at);
		free(line);
	} else {
		dev = co_dev_create_from_dcf_file(ifname);
	}
	if (!dev)
		goto errror_create_dev;

	size_t n = snprintf_c99_sdev(NULL, 0, dev);
	char *s = malloc(n + 1);
	if (n && !s) {
		diag(DIAG_ERROR, get_errc(), "unable to allocate string");
		goto error_malloc_s;
	}
	snprintf_c99_sdev(s, n + 1, dev);

	FILE *stream = stdout;
	if (ofname) {
		stream = fopen(ofname, "w");
		if (!stream) {
			diag(DIAG_ERROR, get_errc(),
					"unable to open %s for writing",
					ofname);
			goto error_fopen;
		}
	}

	fprintf(stream,
			"#include <lely/co/sdev.h>\n\n"
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
error_getdelim:
error_arg:
	return EXIT_FAILURE;
}
