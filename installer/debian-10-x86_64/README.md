This directory contains steps and scripts for how to setup environment for
building FBOSS, build FBOSS binaries and install on a switch using Debian 10

<a name="building">

# 1. Building FBOSS

</a>

This section covers how to build FBOSS binaries and all its library
dependencies. It involves creating a Virtual Machine environment, building
FBOSS and then creating RPM package that contains FBOSS binaries and library
dependencies.

**Requirements**: Ability to a create Virtual Machine that has Internet access.

## 1.1 Setup build VM with Internet connectivity

- Download ISO: Debian 10 x86_64 from:
  https://cdimage.debian.org/cdimage/archive/latest-oldstable/amd64/iso-cd/debian-10.12.0-amd64-netinst.iso
- Create a Virtual Machine(VM) using this CentOS ISO Image.
- The VM can be created using any virtualization software e.g. VMware Fusion or
  Virtual Box, Parallels etc.
- Recommended VM configuration: Disk space: 256Gb, RAM: 16G, CPUs: 16.
  More RAM/CPUS the merrier (faster builds).
- Setup Internet connectivity for the VM.


This VM will be the build environment.

## 1.2 Get FBOSS

```
sudo apt-get install git
git clone https://github.com/facebook/fboss.git fboss.git

```

## 1.3 Install tools

Install tools needed for bulding FBOSS software.

```
cd fboss.git
./installer/debian-10-x86_64/install-tools.sh
```

## 1.4 Build FBOSS binaries and dependencies

</a>

Optionally enable building with address sanitizer options turned on
```
export WITH_ASAN=1
```
Optionally configure option to install benchmark binaries. Requires larger disk space.
```
export BENCHMARK_INSTALL=1
```
Build Dependencies (one time)
```
cd fboss.git
./build/fbcode_builder/getdeps.py install-system-deps --recursive fboss # onetime
```
Build all fboss binaries
```
./build/fbcode_builder/getdeps.py build --allow-system-packages fboss
```
Build only a specific fboss binary/library by passing `--cmake-target`. For example, if you want to build only `platform_manager` binary
```
./build/fbcode_builder/getdeps.py build --allow-system-packages fboss --cmake-target platform_manager
```
