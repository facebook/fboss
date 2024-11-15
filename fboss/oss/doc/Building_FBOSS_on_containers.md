<a name="building">

# Building FBOSS on Docker containers

</a>

This document covers how to build FBOSS binaries and all its library
dependencies on Docker containers.

## Stop any existing containers and clean docker artifacts (optional)

This isn't a strictly required step, although in some cases you may want to
build a completely fresh container to use, in which case the below commands
will stop any existing docker containers and clean the image cache so that the
subsequent steps will build the container from scratch.

```
sudo docker container kill -a && sudo docker container prune -f
sudo docker image prune -af
```

## Building the FBOSS Docker image

The FBOSS GitHub repository contains a Dockerfile that can be used to create
the Docker container image for building FBOSS binaries. The Dockerfile is
located under `fboss/oss/docker/Dockerfile`. You can use the below steps to
build the docker image from this file. Note that the path to the docker file
is relative, so the below commands assume you are currently running them from
the root of the FBOSS git repository.

```
# This builds a docker container image that is tagged as fboss_docker:latest.
# The Dockerfile will set up most of the packages required to build FBOSS OSS.
sudo docker build . -t fboss_docker -f fboss/oss/docker/Dockerfile

# This command starts the docker container from the image built in the previous
# step, and simply runs a detached bash shell.
# NOTE: if you are building against a precompiled SDK, you will need to add
# some additional parameters to this command. See the section below on
# "Building against a precompiled SDK".
sudo docker run -d -it --name=FBOSS_DOCKER_CONTAINER fboss_docker:latest bash

# Attaches our current terminal to a new bash shell in the docker container so
# that we can perform the build within it.
sudo docker exec -it FBOSS_DOCKER_CONTAINER bash
```

Once you've executed the above commands, you should be dropped into a root
shell within the docker container and can proceed with the next steps to start
the build.

## Building FBOSS Binaries

Instructions for building FBOSS binaries may have slight differences based on
which SDK you are linking against (more specifically

### Building Fake SAI binaries

The fake SAI sources are included in the FBOSS git repository, and therefore
don't require any additional steps. You can start the build by using the
commands below:


```
export BUILD_SAI_FAKE=1
time ./build/fbcode_builder/getdeps.py build --allow-system-packages \
--extra-cmake-defines='{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}' \
--scratch-path /var/FBOSS/tmp_bld_dir fboss
```

### Building against a precompiled SDK

This section assumes that you have a precompiled SDK library which you want to
link against. More specifically, you'll need the static library `libsai_impl.a`
for the SDK which you are trying to link against, as well as the associated set
of SAI headers. In order to run the build:

#### Making the SDK artifacts available within the container

You can mount directories from you VM to the container in order to make the SDK
artifacts available. This is done during the run step:

```
# Assuming the existence of /path/to/sdk/lib/libsai_impl.a and /path/to/sdk/include/*.h
for the static library and headers respectively, the below command will mount
those paths to /opt/sdk/lib/libsai_impl.a and /opt/sdk/include/*.h respectively.
sudo docker run -d -v /path/to/sdk:/opt/sdk:z -it --name=FBOSS_DOCKER_CONTAINER fboss_docker:latest bash

```

With the SDK artifacts mounted into the container, you can now perform the build:


```
# Run the build helper to stage the SDK in preparation for the build step.
./fboss/oss/scripts/build-helper.py /opt/sdk/lib/libsai_impl.a /opt/sdk/include/ /var/FBOSS/sai_impl_output


# Run the build
cd /var/FBOSS/fboss

# SDK specific environment variable should be set
export BRCM_SAI_IMPL=1

# Start the build
time ./build/fbcode_builder/getdeps.py build --allow-system-packages \
--extra-cmake-defines='{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}' \
--scratch-path /var/FBOSS/tmp_bld_dir fboss
```
