This directory contains quick steps to build FBOSS OSS for BRCM-SAI.
Please refer READMEs inside installer/$linux-distro for more detailed versions.

# Steps

## One time

```
export linux-distro=centos-8-x64_64 # or debian-10-x86_64
sudo apt-get install git
cd $HOME
git clone https://github.com/facebook/fboss.git fboss.git
cd fboss.git
./installer/$linux-distro/install-tools.sh
./build/fbcode_builder/getdeps.py install-system-deps --recursive fboss
```

## Set up BRCM-SAI implementation dependency for FBOSS

```
rm -rf $HOME/brcm-sai
mkdir $HOME/brcm-sai
cp libsai_impl.a $HOME/brcm-sai/ # Copy the libsai.a from BRCM-SAI build and rename as libsai_impl.a
cp -r experimental $HOME/brcm-sai
cd $HOME/fboss.git/installer/$linux-distro
./build-helper.py $HOME/brcm-sai/build/lib/libsai_impl.a $HOME/brcm-sai/experimental/ $HOME/sai_impl_output
```

## Build FBOSS with BRCM-SAI implementation

```
export SAI_ONLY=1
export SAI_BRCM_IMPL=1
export scratch_dir=$HOME/build_dir # path to store build files
./build/fbcode_builder/getdeps.py build --scratch-path $scratch_dir --allow-system-packages fboss
```

## Switch FBOSS and dependency sources to latest stable commits

```
cd $HOME/fboss.git
rm -rf $scratch_dir # remove existing build dir if any
rm -rf build/deps/github_hashes/facebook
rm -rf build/deps/github_hashes/facebookincubator
tar -xvf fboss/oss/stable_commits/latest_stable_hashes.tar.gz
```

## Make changes to FBOSS source code

```
cd $scratch_dir/repos/github.com-facebook-fboss.git
# make code changes
```

## Make changes to BRCM-SAI source code and build
  TODO

# Workflows

## Workflow: Building latest FBOSS
  Follow steps in:
  - One time
  - Build FBOSS with BRCM-SAI implementation

## Workflow: Building latest stable FBOSS
  Follow steps in:
  - One time
  - Switch FBOSS and dependency sources to latest stable commits
  - Build FBOSS with BRCM-SAI implementation

## Workflow: Iterate on FBOSS changes
  Follow steps in:
  - Make changes to FBOSS source code
  - Build FBOSS with BRCM-SAI implementation

## Workflow: Iterate on BRCM-SAI changes
  Follow steps in:
  - Make changes to BRCM-SAI source code and build
  - Set up BRCM-SAI implementation dependency for FBOSS
  - Build FBOSS with BRCM-SAI implementation
