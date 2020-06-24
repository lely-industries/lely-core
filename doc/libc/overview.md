C11 and POSIX compatibility library overview
============================================

The C11 and POSIX compatibility library provides:
- compiler feature definitions: lely/features.h
- atomics: lely/libc/stdatomic.h
- `max_align_t`: lely/libc/stddef.h
- `getdelim()`, `getline()`, `asprintf()` and `vasprintf()`: lely/libc/stdio.h
- `aligned_alloc()`, `aligned_free()` and `setenv()`: lely/libc/stdlib.h
- `noreturn`: lely/libc/stdnoreturn.h
- `strdup()`, `strndup()` and `strnlen()`: lely/libc/string.h
- `ffs()`, `strcasecmp()` and `strncasecmp()`: lely/libc/strings.h
- `clockid_t`, `ssize_t`: lely/libc/sys/types.h
- threads: lely/libc/threads.h
- `struct timespec`, `nanosleep()`, `timespec_get()` and POSIX Realtime
  Extensions: lely/libc/time.h
- `char16_t` and `char32_t`: lely/libc/uchar.h
- `getopt()`: lely/libc/unistd.h