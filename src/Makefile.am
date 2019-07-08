SUBDIRS = libc tap util can co
if PLATFORM_LINUX
SUBDIRS += io
endif
if PLATFORM_WIN32
SUBDIRS += io
endif
if PLATFORM_LINUX
SUBDIRS += aio
if !NO_CXX
SUBDIRS += coapp
endif
endif

EXTRA_DIST =
EXTRA_DIST += CPPLINT.cfg
