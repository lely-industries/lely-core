if ECSS_COMPLIANCE
SUBDIRS = libc tap util can co
else
SUBDIRS = libc tap util can ev io2 co
if PLATFORM_LINUX
SUBDIRS += io
endif
if PLATFORM_WIN32
SUBDIRS += io
endif
if !NO_CXX
SUBDIRS += coapp
endif
endif
