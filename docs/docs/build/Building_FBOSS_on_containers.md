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

```
git clone https://github.com/facebook/fboss.git
cd fboss
```

## Set Up the FBOSS Docker Image

### Stop Existing Containers and Clean Docker Artifacts

This isn't a strictly required step, although in some cases you may want to
build a completely fresh container to use. The below commands will stop any
existing Docker containers and clean the image cache so that the subsequent
steps will build the container from scratch:

```
sudo docker container kill -a && sudo docker container prune -f
sudo docker image prune -af
```

### Build the FBOSS Docker Image

The FBOSS GitHub repository contains a Dockerfile that can be used to create
the Docker container image for building FBOSS binaries. The Dockerfile is
located under `fboss/oss/docker/Dockerfile`. Use the below steps to
build the image, which installs required dependencies. Note that the path
to the Dockerfile is relative, so the below commands assume you are currently running
them from the root of the repository:

```
# Builds a docker container image that is tagged as fboss_docker:latest
sudo docker build . -t fboss_docker -f fboss/oss/docker/Dockerfile
```

**If you are using fake SAI, continue reading this step. If you are building against a precompiled
SDK, proceed to the next step. There will be different instructions in the "Build Against a Precompiled
SDK" step for starting the container.**

Start the container:

```
# Starts the docker container using the previously built image and runs a detached bash shell
sudo docker run -d -it --name=FBOSS_DOCKER_CONTAINER fboss_docker:latest bash

# A full FBOSS build may take significant space (> 50GB of storage), in which
# case you may want to choose a more specific location for the output (e.g. a
# disk with more space). You can do this by mounting a volume inside the
# container using the `-v` flag as shown below. In this example,
# the path `/opt/app/localbuild` from the base VM is mounted in the container
# as /var/FBOSS/tmp_bld_dir
sudo docker run -d -v /opt/app/localbuild:/var/FBOSS/tmp_bld_dir:z -it \
--name=FBOSS_DOCKER_CONTAINER fboss_docker:latest bash
```

Enter the container:

```
# Attaches our current terminal to a new bash shell in the docker container so
# that we can perform the build within it
sudo docker exec -it FBOSS_DOCKER_CONTAINER bash
```

Once you've executed the above commands, you should be dropped into a root
shell within the Docker container and can proceed with the next steps to start
the build.

### Load the Published Docker Image from GitHub

**Skip this step if the previous build step worked, as this is not preferred.**

Instead of building the Docker image yourself, you can also obtain an image
tarball from GitHub (e.g. https://github.com/facebook/fboss/actions/runs/14961317578).
The advantage of using this image instead of building it yourself is that the
published image includes all relevant sources, including sources for dependencies,
which means that building FBOSS binaries from this image eliminates the need for
a network connection at the time of building (which would normally be required in
order to download sources or clone Git repositories for those same dependencies).

In order to use this image, you can download it from GitHub and decompress it using `zstd`:

```
zstd -d fboss_debian_docker_image.tar.zst
```

You can then load the image via:

```
sudo docker load < fboss_debian_docker_image.tar
```

You can then start the container using the same `docker run` and `docker exec`
command as mentioned above.

## Build FBOSS Binaries

Instructions for building FBOSS binaries may have slight differences based on
which SDK you are linking against.

### Build Using Fake SAI

**Skip to the next section if you are building against a precompiled SDK.**

The fake SAI sources are included in the FBOSS git repository, and therefore
don't require any additional steps. You can start the build immediately by using the
commands below:

```
export BUILD_SAI_FAKE=1
time ./build/fbcode_builder/getdeps.py build --allow-system-packages \
--extra-cmake-defines='{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}' \
--scratch-path /var/FBOSS/tmp_bld_dir fboss
```

### Build Against a Precompiled SDK

This section assumes that you have a precompiled SDK library which you want to
link against. More specifically, you'll need the static library `libsai_impl.a`
for the SDK which you are trying to link against, as well as the associated set
of SAI headers. In order to run the build:

#### Make the SDK Artifacts Available Within the Container

You can mount directories from your VM to the container in order to make the SDK
artifacts available. This is done during the run step:

```
# Assuming the existence of /path/to/sdk/lib/libsai_impl.a and /path/to/sdk/include/*.h
# for the static library and headers respectively, the below command will mount
# those paths to /opt/sdk/lib/libsai_impl.a and /opt/sdk/include/*.h respectively
sudo docker run -d -v /path/to/sdk:/opt/sdk:z -it --name=FBOSS_DOCKER_CONTAINER fboss_docker:latest bash

# A full FBOSS build may take significant space (> 50GB of storage), in which
# case you may want to choose a more specific location for the output (e.g. a
# disk with more space). You can do this by mounting a volume inside the
# container using the `-v` flag as shown below. In this example,
# the path `/opt/app/localbuild` from the base VM is mounted in the container
# as /var/FBOSS/tmp_bld_dir
sudo docker run -d -v /path/to/sdk:/opt/sdk:z \
-v /opt/app/localbuild:/var/FBOSS/tmp_bld_dir:z -it \
--name=FBOSS_DOCKER_CONTAINER fboss_docker:latest bash
```

Enter the container:

```
# Attaches our current terminal to a new bash shell in the docker container so
# that we can perform the build within it
sudo docker exec -it FBOSS_DOCKER_CONTAINER bash
```

#### Run the Build Helper

By default, `build-helper.py` will use SAI version 1.14.0. If you are planning
on building against a different version of SAI, you must add another param to the
build-helper.py command.

Supported values:

1. 1.13.2
1. 1.14.0
1. 1.15.0
1. 1.15.3
1. 1.16.0

```
# Run the build helper to stage the SDK in preparation for the build step
./fboss/oss/scripts/build-helper.py /opt/sdk/lib/libsai_impl.a /opt/sdk/include/ /var/FBOSS/sai_impl_output

# Run the build helper using SAI version 1.15.3 instead of the default 1.14.0
./fboss/oss/scripts/build-helper.py /opt/sdk/lib/libsai_impl.a /opt/sdk/include/ /var/FBOSS/sai_impl_output 1.15.3
```

#### Set Important Environment Variables

The following environment variables should be set depending on which platform and SDK version you are building:

1. `SAI_BRCM_IMPL` - set to 1 if building against brcm-sai SDK
1. `SAI_SDK_VERSION` - Should be set to a string depending on which version of the brcm-sai SDK you are
building. Supported values can be found in [SaiVersion.h](https://github.com/facebook/fboss/blob/main/fboss/agent/hw/sai/api/SaiVersion.h)
but are listed below for convenience. Default value is "SAI_VERSION_11_0_EA_DNX_ODP", found in
[CMakeLists.txt](https://github.com/facebook/fboss/blob/main/CMakeLists.txt#L108)
    - `SAI_VERSION_8_2_0_0_ODP`
    - `SAI_VERSION_8_2_0_0_SIM_ODP`
    - `SAI_VERSION_9_2_0_0_ODP`
    - `SAI_VERSION_9_0_EA_SIM_ODP`
    - `SAI_VERSION_10_0_EA_ODP`
    - `SAI_VERSION_10_0_EA_SIM_ODP`
    - `SAI_VERSION_10_2_0_0_ODP`
    - `SAI_VERSION_11_0_EA_ODP`
    - `SAI_VERSION_11_0_EA_SIM_ODP`
    - `SAI_VERSION_11_3_0_0_ODP`
    - `SAI_VERSION_11_7_0_0_ODP`
    - `SAI_VERSION_10_0_EA_DNX_ODP`
    - `SAI_VERSION_10_0_EA_DNX_SIM_ODP`
    - `SAI_VERSION_11_0_EA_DNX_ODP`
    - `SAI_VERSION_11_0_EA_DNX_SIM_ODP`
    - `SAI_VERSION_11_3_0_0_DNX_ODP`
    - `SAI_VERSION_11_7_0_0_DNX_ODP`
    - `SAI_VERSION_12_0_EA_DNX_ODP`
    - `SAI_VERSION_13_0_EA_ODP`
1. `SAI_VERSION` - can be omitted if you are using SAI 1.14.0. If using a more
recent version of SAI from https://github.com/opencomputeproject/SAI, this should be set to the semantic version e.g. 1.16.1.

#### Build Against the SDK

Ensure you are in the right directory, set your relevant environment variables, and start the build:

```
# Navigate to the right directory
cd /var/FBOSS/fboss

# Set environment variables according to the previous step
export SAI_BRCM_IMPL=1

# Start the build
time ./build/fbcode_builder/getdeps.py build --allow-system-packages \
--extra-cmake-defines='{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}' \
--scratch-path /var/FBOSS/tmp_bld_dir fboss
```

### Limit the Build to a Specific Target

You can limit the build to a specific target by using the `--cmake-target` flag.
Buildable targets can be found by examining the cmake scripts in the repository.
Any buildable target will be specified in the cmake scripts either by
`add_executable` or `add_library`. Example command below:

```
time ./build/fbcode_builder/getdeps.py build --allow-system-packages \
--extra-cmake-defines='{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}' \
--scratch-path /var/FBOSS/tmp_bld_dir --cmake-target qsfp_service fboss
```
