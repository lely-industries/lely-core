# Lely core libraries

The Lely core libraries are a collection of C and C++ libraries and tools,
providing hih-performance I/O and sensor/actuator control for robotics and IoT
applications. The libraries are cross-platform and have few dependencies. They
can be even be used on bare-metal microcontrollers with as little as 32 kB RAM.

## Overview

The Lely core libraries consist of:
- C11 and POSIX compatibility library (liblely-compat)
- Test Anything Protocol (TAP) library (liblely-tap)
- Utilities library (liblely-util)
- Event library (liblely-ev)
- Asynchronous I/O library (liblely-io2)
- CANopen library (liblely-co)
- C++ CANopen application library (liblely-coapp)

Click [here](https://opensource.lely.com/canopen/docs/overview/) for more
details.

## Getting started

### Download

Pre-built Debian packages are available on our
[Ubuntu PPA](https://launchpad.net/~lely/+archive/ubuntu/ppa).

You can download the source code from the
[Releases](https://gitlab.com/lely_industries/lely-core/-/releases) page, or
clone this repository with

    $ git clone https://gitlab.com/lely_industries/lely-core.git

### Build and install

This project uses the GNU Build System (`configure`, `make`, `make install`),
available on Linux and Windows (through [Cygwin](https://www.cygwin.com/)). To
build the libraries and tools, you need to install the autotools (autoconf,
automake and libtool). After the initial clone or download of the source,
generate the `configure` script by running

    $ autoreconf -i

in root directory of the project. This step only has to be repeated if
`configure.ac` or one the the `Makefile.am` files changes.

First, configure the build system by running

    $ ./configure --disable-python

If you do not want to clutter the source directories with object files, you can
run `configure` from another directory. `--disable-python` disables the
deprecated Python bindings. The `configure` script supports many other options.
The full list can be shown with

    $ ./configure --help

and is documented
[here](https://opensource.lely.com/canopen/docs/configuration/).

Once the build system is configured, the libraries and tools can be built with

    $ make

The optional test suite can be run with

    $ make check

If you have [doxygen](http://www.doxygen.org/) and
[Graphviz](http://www.graphviz.org/) installed, you can build the HTML
documentation of the API with

    $ make html

Finally, install the binaries, headers and documentation by running

    # make install

as root.

Click [here](https://opensource.lely.com/canopen/docs/installation/) for more
information about building the lely-core libraries from source, as well as
instructions for cross-compilation.

For more details on setup and tools used for the development of the library,
you can check the `docker/` folder for all images used by the Continuous
Integration system [configuration](./.gitlab-ci.yml).
See [docker/README.md](docker/README.md) more for details.

## ECSS Compliance

The project is compliant with [ECSS](https://ecss.nl/) requirements
for space flight software criticality B.

To enable compliance add `--enable-ecss-compliance` to build configuration.

Only liblely-compat, liblely-util and liblely-co are included in
the ECSS compliance mode.

For criticality B validation Test Suite visit
[Test Suite](https://gitlab.com/n7space/canopen/test-suite).

## Documentation

The doxygen-generated API documentation of the latest development version can be
found [here](http://lely_industries.gitlab.io/lely-core/doxygen/).

## Licensing

Copyright 2013-2021 [Lely Industries N.V.](http://www.lely.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
