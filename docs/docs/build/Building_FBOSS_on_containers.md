---
id: building_fboss_on_docker_containers
title: Building FBOSS on Docker Containers
description: How to build FBOSS OSS services in a reproducible environment
keywords:
    - FBOSS
    - OSS
    - build
    - docker
oncall: fboss_oss
---
This document covers how to build FBOSS binaries and all its library
dependencies on Docker containers.

## Clone the FBOSS Repository
We will be working out of the root of the repository:

```bash file=./static/code_snips/clone_fboss.sh
```

## Set Up the FBOSS Container

### Stop Existing Containers and Clean Docker Artifacts

This isn't a strictly required step, although in some cases you may want to
build a completely fresh container to use. The below commands will stop any
existing Docker containers and clean the image cache so that the subsequent
steps will build the container from scratch:

```bash file=./static/code_snips/clean_docker.sh
```

### Build the FBOSS Docker Image

The FBOSS GitHub repository contains a Dockerfile that can be used to create
the Docker container image for building FBOSS binaries. The Dockerfile is
located under `fboss/oss/docker/Dockerfile`. Use the below command to
build the image, which installs required dependencies. Note that the path
to the Dockerfile is relative, so the below commands assume you are currently running
them from the root of the repository:

```bash file=./static/code_snips/build_docker_image.sh
```

## Start the FBOSS Container

### Start a Container for Platform Stack

```bash file=./static/code_snips/start_platform_stack_container.sh
```

### Start a Container for Forwarding Stack

```bash file=./static/code_snips/start_forwarding_stack_container.sh
```

### Enter the Container

```bash file=./static/code_snips/enter_docker_container.sh
```

At this point, you should be dropped into a root shell within the Docker container
and can proceed with the next steps to start the build.

## Build FBOSS Binaries

Instructions for building FBOSS binaries may have slight differences based on
which SDK you are linking against.

### Build Platform Stack

You only need one command to build the Platform Stack, where `$TARGET` is
`fboss_platform_services`:

```bash file=./static/code_snips/build_with_cmake_target.sh
```

### Build Forwarding Stack

This section assumes that you have a precompiled ASIC SDK library which you want to
link against. More specifically, you'll need the static library `libsai_impl.a`
for the SDK which you are trying to link against, as well as the associated set
of SAI headers. In order to run the build:

#### Run the Build Helper

```bash file=./static/code_snips/build_helper.sh
```

#### Set Important Environment Variables

```bash file=./static/code_snips/important_environment_variables.sh
```

#### Build Against the SDK

Ensure you are in the right directory, set your relevant environment variables,
and start the build:

```bash file=./static/code_snips/build_forwarding_stack.sh
```

#### Build QSFP Targets Against PHY SAI SDK

For some project that needs to use XPHY(Retimer) which has its own SAI SDK, we need
to build QSFP targets to link with the precompiled PHY SDK library just like we did
above for Agent targets linking with ASIC SDK. However, we won't be able to support
building both Agent and QSFP targets with the single `getdeps.py build fboss` command
like above, mainly because ASIC SAI SDK and PHY SAI SDK usually don't use the same SAI
version and implementation.
Therefore, for any platform that needs linking both ASIC SAI SDK and PHY SAI SDK,
we highly recommend to do it in the following order:
- Run the commands from [Build Against the SDK](#build-against-the-sdk) to build Agent
  and other services
- Run the following command to build QSFP targets

```bash file=./static/code_snips/build_qsfp_targets_with_brcm_pai.sh
```

### Limit the Build to a Specific Target

You can limit the build to a specific target by using the `--cmake-target` flag.
Buildable targets can be found by examining the cmake scripts in the repository.
Any buildable target will be specified in the cmake scripts either by
`add_executable` or `add_library`. Example command below:

```bash file=./static/code_snips/build_with_cmake_target.sh
```
