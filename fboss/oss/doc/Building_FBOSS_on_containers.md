<a name="building">

# Building FBOSS on Docker containers

</a>

This document covers how to build FBOSS binaries and all its library
dependencies on Docker containers.

## Build system requirements

Any VM (preferably CentOS 8) that has Internet access


## Initial setup on VM

Setup the environmental variables based on the container distro (preferably CentOS 8) -

```
# For CentOS Stream 8 build (preferred)
export CONTAINER_TAR=fboss_centos8.tar
export DOCKER_IMAGE=fboss_centos8
export DOCKER_NAME=FBOSS_CENTOS8

# For Debian 10 Buster build
export CONTAINER_TAR=fboss_buster.tar
export DOCKER_IMAGE=fboss_buster
export DOCKER_NAME=FBOSS_BUSTER
```

## Download container images

- Create /opt/app/FBOSS_DIR directory in VM (sudo mkdir -p /opt/app/FBOSS_DIR)
- Download $CONTAINER_TAR:- git clone https://fboss@bitbucket.org/fboss/build-containers.git
- Copy build-containers/centos/fboss_centos8.tar to host VM in /opt/app/FBOSS_DIR/


## Create docker container

```
sudo yum install -y docker
sudo cd /opt/app/FBOSS_DIR
sudo docker import $CONTAINER_TAR
sudo docker images # $IMAGE_ID = IMAGE ID from this command output
sudo docker tag $IMAGE_ID $DOCKER_IMAGE:latest
sudo docker images
# /opt/app/FBOSS_DIR from VM is mapped to /var/FBOSS on container
sudo docker run -d -v /opt/app/FBOSS_DIR:/var/FBOSS:z -it --name=$DOCKER_NAME $DOCKER_IMAGE bash
sudo docker exec -it $DOCKER_NAME bash # drops to container shell
```

## Build ASIC vendor’s SAI SDK library
- Copy the SAI SDK sources from the ASIC vendor to host VM - /opt/app/FBOSS_DIR/
- Build the SAI SDK library on the container (/var/FBOSS/ in container is mapped
  to /opt/app/FBOSS_DIR/ in host VM)
- The SAI experimental headers in SAI SDK source and the libsai.a built in the
  above step would be used later to build FBOSS binaries and link with SAI library


## Build ASIC vendor's SDK kmods
- Copy the target switch's kernel version headers to VM - /opt/app/FBOSS_DIR/
- Build SDK kmods using the copied kernel headers


## Clone FBOSS sources and switch to latest stable commit

From container running on VM -

```
cd /var/FBOSS/
git clone https://github.com/facebook/fboss fboss.git
cd /var/FBOSS/fboss.git
rm -rf $scratch_dir # remove existing build dir if any
# Optionally, pin the fboss and its dependencies to known
# stable commit hash
rm -rf build/deps/github_hashes/facebook
rm -rf build/deps/github_hashes/facebookincubator
tar -xvf fboss/oss/stable_commits/latest_stable_hashes.tar.gz
```

## Prepare to build FBOSS

This step involves copying the libsai.a from ASIC vendor SAI SDK build and SAI experimental headers from SAI SDK sources and then running FBOSS’ build helper script.

From container running on VM -

```
rm -rf /var/FBOSS/built-sai; mkdir -p /var/FBOSS/built-sai/experimental
cp <asic_vendor_build_output>/libsai.a /var/FBOSS/built-sai/libsai_impl.a
cp <asic_vendor_sdk>/include/experimental/*.* /var/FBOSS/built-sai/experimental
cd /var/FBOSS/fboss.git/fboss/oss/scripts
./build-helper.py /var/FBOSS/built-sai/libsai_impl.a /var/FBOSS/built-sai/experimental/ /var/FBOSS/sai_impl_output
```

## Build FBOSS

From container running on VM -

```
export SAI_ONLY=1
export SAI_BRCM_IMPL=1 # Needed only for BRCM SAI
export GETDEPS_USE_WGET=1
cd /var/FBOSS/fboss.git
time ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss
```

### Building a specific FBOSS target

Instead of building all FBOSS OSS binaries, a specific binary can be built using "--cmake-target" option.

For building the SAI HW test binary only, the following command can be used -

```
time ./build/fbcode_builder/getdeps.py build --allow-system-packages --scratch-path /var/FBOSS/tmp_bld_dir fboss --cmake-target sai_test-sai_impl-1.11.0
```

NOTE: If "--cmake-target" option is used, then the binary will not be installed and hence will be available only in <scratch_patch>/build/fboss/ directory.
