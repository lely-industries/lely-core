# Lely core libraries - development docker images

This folder contains docker images configurations used by Continous Integration
system to build and validate lely-core libraries. The same contaniners can be
used by developers to quickly set up local development environment.


## Folder structure

Instead of a single huge image, various tools use separate images.
It makes images smaller but also helps isolating various validations.
Each image configuration file (Dockerfile) is stored in separate folder.

Dockerfiles are organized in following folder structure:
 * `<tool>/Dockerfile` - for tools that have dedicated base container
                         (e.g. gcc)
 * `<tool>/<distribution>/Dockerfile` - for tools that needs container
                                        built from scratch image of
                                        selected distribution

The `<tool>` name corresponds to the CI task defined in `.gitlab-ci.yml`,
usually name of the tasks include the name of the used container.
The CI setup can be treated as instruction on images usage.

Some of the images that needs to be built from scratch have a common base stored in
`build/<distribution>` which allows to simply share layers between images.


## GitLab.com Docker Repository

All images used by project's CI system can be directly downloaded from
project's GitLab.com Docker Registry.

To browse available images go to:
`Project GitLab.com page > Packages & Registries > Registry`.

Each available image has an icon which allows to copy the direct link
to the selected image and pass it to Docker commands.

For example:

`docker run -ti registry.gitlab.com/n7space/canopen/lely-core/lcov:bullseye bash`

will download `lcov` tool's image and run interactive bash in it.

NOTE: Depending on user's settings, the image might need to be manually pulled from
the registry (using `docker pull`) before executing `docker run`.


## Using tools from images

To execute tools provided by containers `docker run` command needs to be used.

For tools that require multiple commands to be called, user can:
 * call one command at a time, prefixing each with `docker run <IMAGE>`,
 * create shell inside container by calling `docker run -ti <IMAGE> bash`
   and executing all commands in that shell.

It is recommended in second approach and necessary in the first one to mount
local folder inside container using `-v <local path>:<container path>` switch.

Switching working directory using `--workdir <container path>` can be helpful.

It is worth noting, that by default docker will run container as `root` user,
so all files created by container in mounted volume/directory will be owned by
`root`, which might be incovinient. One of the solutions is to pass
`--user $(id -u):$(id -g)` switch when calling build commands.

In both cases `--rm` switch will help save disk space.

For example:

``` bash
docker run \
    -v $PWD:/home/dev/lely-core \
    --workdir /home/dev/lely-core \
    --user $(id -u):$(id -g) \
    --rm \
    lely-core/gcc:10 \
    autoreconf -i
```

will execute autotools autoreconf inside GCC 10 image
using current directory as working directory.
For this command to succeed, the current directory should be
the root folder of the repository checkout.

To use some features of the library setting up the environment variables is required. It can be achieved by creating a file named, for example, `env-variables.var` containing the entries in the following format:
```
VARIABLE_NAME1=VALUE1
VARIABLE_NAME2=VALUE2
```
After that the option `--env-file env-variables.var` can be added to the `docker run` command to load these values into the container.

## Building image locally

Dockerfiles are prepared for building images targeting CI and Docker Registry,
so building them locally does not follow common Docker patterns.

  * Images assume they are downloaded from registry, so names should be prefixed
    with some `prefix/` and `REGISTRY=prefix` argument passed while building.
  * Some tools require additional arguments (like tool version, e.g. gcc).

Example - building _lcov_ image, which depends on _build_ image.

  1. In `build/bullseye/` : `docker build -t lely-core/build:bullseye .`
  2. In `lcov/bullseye/` : `docker build -t lely-core/lcov:bullseye --build-arg REGISTRY=lely-core .`

Example - building _gcc_ image for GCC 10.

  In `gcc/` : `docker build -t lely-core/gcc:10 --build-arg __GNUC__=10 .`
