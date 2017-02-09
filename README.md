![logo](doc/logo.png)

C11 and POSIX compatibility library
===================================

Introduction
------------

The Lely C11 and POSIX compatibility library ([liblely-libc]) makes a selection
of C11 features available in C99 (and C++98) and implements certain C99 and
POSIX features that may be missing on some platforms. The library is intended to
provide platform independence to C99- and C11-compliant users.

Download
--------

liblely-libc is hosted on [GitLab]. You can clone the repository with

    $ git clone https://gitlab.com/lely_industries/libc.git

or download the latest release
([v1.4.0](https://gitlab.com/lely_industries/libc/tags/v1.4.0)).

Binary packages for Ubuntu 16.04 LTS (Xenial Xerus) can be found at
https://launchpad.net/~lely/+archive/ubuntu/ppa.

Build and install
-----------------

liblely-libc uses the GNU Build System (`configure`, `make`, `make install`) and
has native support for POSIX platforms (Debian-based Linux distributions in
particular) and Windows (Vista and higher) through [Cygwin] or [MinGW-w64].
[GCC ARM Embedded] and Microsoft Visual Studio (2013 and higher) are also
supported, but project files will have to be created by hand.

The release archive includes the build system, but if you want to build a
checkout of the repository, you need to install the autotools (autoconf,
automake and libtool). The build system can then be generated by running

    $ autoreconf -i

in the root directory of the repository.

If you want to generate HTML documentation from the source code, you need to
install [doxygen] and [Graphviz].

### Debian packages

For Debian-based Linux distributions, the preferred installation method is to
generate and install Debian packages. Install the Debian package build system
with

    $ sudo apt-get install cdbs devscripts

The packages can now be built by running

    $ debuild -uc -us -b

from the root directory of the project. Once the packages are generated, you can
clean up the project directory with

    $ debuild clean

debuild creates the packages in the parent directory of the project. Install
them with

    $ cd ..
    $ sudo dpkg -i liblely-libc*.deb

### configure, make, make install

It is also possible to build and install the library by hand. First, configure
the build system by running

    $ ./configure

from the root directory of the project.

The `configure` script supports many options. The full list can be shown with

    $ ./configure --help

Once the build system is configured, the library can be built with

    $ make

Run

    $ make html

to generate the HTML documentation from the source code. The binaries, headers
and documentation can be installed by running

    # make install

as root.

### Options

liblely-libc provides an implementation of the C11 multithreading support
library. Multithreading support can be disabled by providing the
`--disable-threads` option to `configure` or by defining the `LELY_NO_THREADS`
preprocessor macro. The implementation is based on pthreads. When building the
library without the autotools build system, define the `LELY_HAVE_PTHREAD`
preprocessor macro if pthreads is available.

Usage
-----

To use the library, include `<lely/libc/header>`, where `header` is the name of
a missing or incomplete C header. See [doc/overview.md](@ref md_doc_overview)
for an overview of the provided functionality. Doxygen documentation for the
latest development version can be found at
http://lely_industries.gitlab.io/libc/doxygen/.

Licensing
---------

Copyright 2017 [Lely Industries N.V.]

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

[Cygwin]: https://www.cygwin.com/
[doxygen]: http://www.doxygen.org/
[GCC ARM Embedded]: https://launchpad.net/gcc-arm-embedded
[GitLab]: https://gitlab.com/lely_industries/libc
[Graphviz]: http://www.graphviz.org/
[Lely Industries N.V.]: http://www.lely.com
[liblely-libc]: https://gitlab.com/lely_industries/libc
[MinGW-w64]: http://mingw-w64.org/

