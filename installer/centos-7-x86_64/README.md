This directory contains steps and scripts for how to setup environment for
building FBOSS, build FBOSS binaries and install on a switch using CentOS 7
x86-64.

<a name="building">

# 1. Building FBOSS

</a>

This section covers how to build FBOSS binaries and all its library
dependencies. It involves creating a Virtual Machine environment, building
FBOSS and then creating RPM package that contains FBOSS binaries and library
dependencies.

**Requirements**: Ability to a create Virtual Machine that has Internet access.

## 1.1 Setup build VM with Internet connectivity

- Download ISO: CentOS 7 x86_64 from: https://www.centos.org/download
- Create a Virtual Machine(VM) using this CentOS ISO Image.
- The VM can be created using any virtualization software e.g. VMware Fusion or
  KVM etc. Refer [Installing VM on VMware Fusion](#fusion-install) section for
  additional details.
- Recommended VM configuration: Disk space: 128Gb, RAM: 16G, CPUs: 8.
  More RAM/CPUS the merrier (faster builds).
- Setup Internet connectivity for the VM
  - From command prompt, run ```ip addr list``` to list interfaces.
  - Request DHCP address on interface. e.g. ```dhclient ens33``` or
    ```dhclient -6 ens33``` (for IPv6)
  - Verify Internet connectivity by ping'ing a known address.
    e.g ```ping internet.org``` or ``` ping6 internet.org``` (for IPv6)
  - **Optionally** configure proxy settings if VM's internet access is through a
    proxy server
      - Add these lines to ```~/.bashrc```
        ```
        export HTTP_PROXY=http://<proxy-server>:<port>
        export HTTPS_PROXY=http://<proxy-server>:<port>
        export http_proxy=http://<proxy-server>:<port>
        export https_proxy=http://<proxy-server>:<port>
        ```
      - Add this line to ```/etc/yum.conf```:
        ```proxy=https://<proxy-server>:<port>```
      - Add this line to ```/etc/sudoers``` (using ```sudo visudo```)

        ```Defaults    env_keep += "ftp_proxy http_proxy https_proxy no_proxy"```

This VM will be the build environment.

## 1.2 Get FBOSS

```
sudo yum install git
git clone https://github.com/facebook/fboss.git fboss.git
```
<a name="upgrade-kernel">

## 1.3 Upgrade kernel

</a>

CentOS 7.0 comes with kernel 3.10. Use the following steps to upgrade the kernel to
a more recent version to match with the kernel used on systems running FBOSS
(currently 4.4.x).

```
cd fboss.git
./installer/centos-7-x86_64/upgrade-kernel.sh
```

Connect to the VM's serial console
```
reboot
```

During reboot choose the newly installed kernel version (default would still be 3.10).

Once the VM has booted up successfully, verify the version of the kernel
```
uname -a
```

Change the default kernel to be booted to the newly installed version
by following these steps

In ```/etc/default/grub``` if ```GRUB_DEFAULT=<number>```, edit it to
```GRUB_DEFAULT=0``` and then execute these commands
```
grub2-mkconfig -o /boot/grub2/grub.cfg
reboot
```

Otherwise in ```/etc/default/grub``` if ```GRUB_DEFAULT=saved```,
then execute these commands
```
grub2-editenv list
grub2-set-default 0
```

Reboot the VM and verify the default kernel being booted has indeed been changed
```
reboot
```

Reference: https://www.tecmint.com/install-upgrade-kernel-version-in-centos-7

## 1.4 Install tools

Install tools needed for bulding FBOSS software.

```
cd fboss.git
./installer/centos-7-x86_64/install-tools.sh
```

<a name="build-fboss">

## 1.5 Build FBOSS binaries and dependencies

</a>

Set up the build environment
```
export CPLUS_INCLUDE_PATH=/opt/rh/rh-python36/root/usr/include/python3.6m/
export PATH=/opt/rh/devtoolset-8/root/usr/bin/:/opt/rh/rh-python36/root/usr/bin:$PATH
```
Optionally enable building with address sanitizer options turned on
```
export WITH_ASAN=1
```
Optionally enable building SAI binaries only
```
export SAI_ONLY=1
```
Optionally configure option to install benchmark binaries. Requires larger disk space.
```
export BENCHMARK_INSTALL=1
```
Build
```
cd fboss.git
./build/fbcode_builder/getdeps.py build fboss
```

The built FBOSS binaries (e.g. ```wedge_agent```, ```bcm_test```) will be installed at
```
/tmp/fbcode_builder_getdeps-ZhomeZ${USER}Zfboss.gitZbuildZfbcode_builder/installed/fboss/bin/
```

### Building SDK Kernel Modules

In order to run FBOSS binaries on a test switch, we would also need to build
SDK kernel modules from sources

#### OpenNSA SDK

Build the OpenNSA kernel modules using the following steps
```
cd fboss.git
./installer/centos-7-x86_64/build-bcm-ko.sh
```

### Build FYI

- The FBOSS build process downloads and builds all FBOSS dependencies listed in
  ```build/fbcode_builder/manifests/fboss``` (and recursively does the same for every
  listed dependency).
- The build time depends on the number of vCPUS and memory configured for the VM.
  e.g. A 32 vCPU VM with 16G RAM and 512M swap space builds FBOSS in
  approximately 20 minutes.
- By default, fbcode_builder will set the number of concurrent build jobs to be
  equal to the number of cpu cores of the VM. This can be overriden by using
  the ```--num-jobs``` cmdline option e.g.
  ```
  cd fboss.git
  ./build/fbcode_builder/getdeps.py build fboss --num-jobs 8
  ```
- If the build fails with the following error, consider the following options
    ```
    c++: fatal error: Killed signal terminated program cc1plus
    ```

  - Check the swap utilization (run ```free -mhb```) and
  consider increasing the number of vcpus/memory/swap size
  - Consider reducing the number of concurrent jobs using ```--num-jobs```

<a name="package">

## 1.6 Packaging FBOSS binaries, dependencies and other build artifacts

</a>

<a name="package-directory">

### 1.6.1 Packaging FBOSS to a directory

</a>

```
cd fboss.git
/opt/rh/rh-python36/root/bin/python3.6 fboss/oss/scripts/package-fboss.py
```

This creates a package directory with prefix **```/tmp/fboss_bins```** and copies all
the essential artifacts of the FBOSS build process (binaries, dependencies,
config files, helper scripts, etc.)  from various locations to it.

This package directory could then be copied over to the test switch for exercising
FBOSS and associated tests.

<a name="package-rpm">

### 1.6.2 Packaging FBOSS to an RPM

</a>

Alternatively, an RPM package can also be built with all the FBOSS build artifacts.
However, it would be orders of magnitude slower than packaging to a directory.

```
cd fboss.git
export QA_RPATHS=$[ 0x0001|0x0002|0x0004|0x0008|0x0010|0x0020 ]
/opt/rh/rh-python36/root/bin/python3.6 fboss/oss/scripts/package-fboss.py --rpm
```

The built RPM would be copied here
```
$HOME/rpmbuild/RPMS/x86_64/fboss_bins-1-1.el7.centos.x86_64.rpm
```

# 2. Installing FBOSS

This section covers how to install FBOSS RPM package built using instructions
described in the previous section, on to a switch.

**Requirements**
- Access to hardware (switch) that can support running FBOSS.
- Serial console access to the switch.
- Facility to copy FBOSS build package from the build VM to the switch.

This section assumes that the switch does not have Internet connectivity.
However, if the switch does have Internet connectivity, some of the
steps could be simplied - e.g. upgrading the kernel on the switch can follow similar
steps as described in [1.3 Upgrade kernel](#upgrade-kernel) for the build VM.

## 2.1 Install CentOS 7.0

This should follow standard instructions for installing a Linux distribution on
a x86_64 machine. In particular, install CentOS 7 x86_64 ISO from:
  https://www.centos.org/download

As an example, see [Installing CentOS 7 from bootable USB](#usb-centos-install).

## 2.2 Upgrade kernel

This section assumes that the switch does not have Internet connectivity.
However, if the switch does have Internet connectivity, some of the
steps could be simplied - e.g. upgrading the kernel on the switch can follow similar
steps as described in [1.3 Upgrade kernel](#upgrade-kernel) for the build VM.

### 2.2.1 Download new kernel rpms

On the CentOS build VM setup in [1. Building](#building), download the kernel RPM
that matches the version running on the VM

Install ```yumdownloader```
```
yum install yum-utils
```
Download kernel rpms to the current directory
```
yumdownloader --enablerepo=elrepo-kernel kernel-lt-`uname -r`
```

### 2.2.2 Install new kernel

- Copy over the downloaded new kernel rpm from the build VM to the switch.
- Install the kernel RPM
  ```
  rpm -ivh kernel-lt-4.4.181-1.el7.elrepo
  ```

Follow instructions in [1.3 Upgrade kernel](#upgrade-kernel) to reboot, verify
and change the grub boot order.


<a name="install-fboss-bins">

## 2.3 Installing FBOSS binaries

</a>

### 2.3.1 Installing from directory package

- From the build VM, copy over the FBOSS package directory (see [Section 1.6.1](#package-directory)) to the switch
    ```
    scp -r /tmp/fboss_bins-<pkg-id> root@$switchName:/opt/
    ```
- On the switch, point  /opt/fboss to the new package before using
    ```
    ln -s  /opt/fboss_bins-<pkg-id> /opt/fboss
    ```

### 2.3.2 Installing RPM

Alternatively, if FBOSS was packaged into an RPM (see [Section 1.6.2](#package-rpm))
- Copy the RPM to the switch.
- Install the RPM
   ```
   rpm -ivh fboss_bins-1-1.el7.centos.x86_64.rpm
   ```
- The binaries, dependent libraries and other package artifacts would be installed to
  ```/opt/fboss/```

## 2.4 Installing Python3

If the switch has Internet connectivity then install python3 using yum installer

```
yum install rh-python36
```

Otherwise (if the switch does not have Internet connectivity)

- On the CentOS build VM setup in [1. Building](#building), download the
python36 and it's dependent packages and copy them over to the switch

  ```
  mkdir /tmp/python36-and-deps; cd /tmp/python36-and-deps
  sudo yum install --downloadonly --downloaddir=/tmp/python36-and-deps --installroot=/tmp/python36-and-deps --releasever=/ rh-python36
  scp -r /tmp/python36-and-deps root@$switchName:/tmp/
  ```

- On the switch, install rh-python36 from the RPM files

  ```
  cd /tmp/python36-and-deps
  sudo rpm -Uvvh --force --nodeps *.rpm
  ```

  - Verify rh-python36 installation
    ```
    /opt/rh/rh-python36/root/usr/bin/python3 -V
    ```
  - Set up the environment to pick up rh-python36
    ```
    source /opt/rh/rh-python36/enable
    ```
  - Set up the environment to pick up rh-python36 on every boot by creating this file
    ```
    [root@wedge ~]# cat /etc/profile.d/enablepython36.sh
    #!/bin/bash
    source /opt/rh/rh-python36/enable
    ```

## 2.5 Install tips

### 2.5.1 Checking dependencies

Verify that all runtime dependencies are satisified including by the libraries
installed in ```/opt/fboss```
```
cd /opt/fboss
source ./bin/setup_fboss_env
ldd /opt/fboss/bin/wedge_agent
ldd /opt/fboss/bin/bcm_test
```

### 2.5.2 Checking, removing, reinstalling RPM

Check for installed FBOSS RPM
```
rpm -qa | grep fboss
```
Remove FBOSS RPM
```
sudo rpm -e fboss_bins-1-1.el7.centos.x86_64
```
Reinstall FBOSS RPM
```
sudo rpm -ivh fboss_bins-1-1.el7.centos.x86_64.rpm
```


# 3. Running FBOSS

Switch to the FBOSS install directory on the switch and set up FBOSS environment
```
cd /opt/fboss
source ./bin/setup_fboss_env
```
Run setup script to populate fruid.json, other config files and install/load
kernel modules
```
./bin/setup.py
```
Run ```/opt/fboss/bin/{wedge_agent, bcm_test}``` etc. with desired arguments. e.g.

```
./bin/bcm_test --bcm_config ./share/bcm_configs/WEDGE100S+RSW-bcm.conf --flexports --gtest_filter=BcmQosPolicyTest.QosPolicyCreate # on Tomahawk
./bin/bcm_test --bcm_config ./share/bcm_configs/MINIPACK+FSW-bcm.conf --flexports --gtest_filter=BcmQosPolicyTest.QosPolicyCreate # on Tomahawk3
```

## 3.1 Running FBOSS Tests
The FBOSS package includes tests that can be exercised on Wedge switches.

### 3.1.1 Running selective FBOSS tests
The test programs installed in ```/opt/fboss/bin (e.g. bcm_test, sai_test-*)``` can be
launched to run tests with a variety of options and filters.
```
cd /opt/fboss
source ./bin/setup_fboss_env
./bin/bcm_test --bcm_config ./share/bcm_configs/WEDGE100S+RSW-bcm.conf --flexports --gtest_filter=BcmQosPolicyTest.QosPolicyCreate
```

### 3.1.2 Running FBOSS BCM tests
FBOSS tests can be launched by executing the test runner script. The script automatically selects and runs tests relevant to the BCM Wedge platform with optional filters.
```
cd /opt/fboss
source ./bin/setup_fboss_env
./bin/run_test.py bcm
./bin/run_test.py bcm --coldboot_only
./bin/run_test.py bcm --filter=*BcmQosMap*
./bin/run_test.py bcm --filter=*Route*V6* --coldboot_only
```

### 3.1.3 Running FBOSS Agent built with SAI
When built with an SAI implementation, FBOSS package also includes the SAI based ```wedge_agent``` binary. The binary can be launched with the platform's configuration file.
```
cd /opt/fboss
source ./bin/setup_fboss_env
./bin/wedge_agent-sai_impl-1.6.3 --config ./share/<sai_platform>_agent.conf --skip_transceiver_programming
```

---
# Incorporating support for a new SAI implementation

The build steps described in [Section 1. Building](#building) build binaries
(Wedge agent, SAI tests) that link with SAI implementation provided by a ```fake_sai``` library e.g. ```sai_test-fake-1.5.0```

To build binaries that link with a new SAI implementation, we need to add the new
SAI implementation as a dependency for FBOSS and specify the way to retrieve it.
This is achieved using steps described below

### Build a static library for the new SAI implementation and name it ```libsai_impl.a``` and copy ```libsai_impl.a``` to a directory named ```lib```
```
mkdir lib
cp libsai_impl.a lib
tar czvf libsai_impl.tar.gz lib/libsai_impl.a
sha256sum libsai_impl.tar.gz
```

### Create a manifest for SAI implementation

```
cat fboss.git/build/fbcode_builder/manifests/sai_impl
```
```
[manifest]
name = sai_impl

[download]
url = http://localhost:8000/libsai_impl.tar.gz
sha256 = <insert the output of 'sha256sum libsai_impl.tar.gz' here>

[build]
builder = nop

[install.files]
lib = lib
```

### Edit FBOSS manifest to add a dependency on the ```sai_impl```

```
diff --git a/build/fbcode_builder/manifests/fboss
b/build/fbcode_builder/manifests/fboss
index e07e1e7..2a1016e 100644
--- a/build/fbcode_builder/manifests/fboss
+++ b/build/fbcode_builder/manifests/fboss
@@ -32,6 +32,7 @@ libnl
 libsai
 OpenNSA
 re2
+sai_impl
```

### From the directory that ```libsai_impl.tar.gz``` is located, start a webserver locally
```
python3 -m http.server
```

### Build FBOSS using steps described in [Section 1.5](build-fboss)

In addition to SAI tests that link with ```fake_sai```,
SAI tests that link with ```sai_impl``` e.g. ```sai_test-sai_impl-1.5.0``` would be
additionally built and installed

## Packaging SAI tests and dependencies for the new SAI implementation

In order for the FBOSS packaging (directory or RPM) to include the newly added SAI tests,
the packaging script and RPM spec have to be modified. If you are not building RPM
package, modifying only the packaging script is enough.

Make the changes as shown below and build the package (see [Section 1.6](#package))

```
diff --git a/fboss/oss/scripts/package-fboss.py b/fboss/oss/scripts/package-fboss.py
index e12b43f..ba5ff87 100755
--- a/fboss/oss/scripts/package-fboss.py
+++ b/fboss/oss/scripts/package-fboss.py
@@ -25,7 +25,7 @@ class BuildRpm:
     DEVTOOLS_LIBRARY_PATH = "/opt/rh/devtoolset-8/root/usr/lib64"

     NAME_TO_EXECUTABLES = {
-        "fboss": (BIN, ["wedge_agent", "bcm_test", "sai_test-fake-1.5.0"]),
+        "fboss": (BIN, ["wedge_agent", "bcm_test", "sai_test-fake-1.5.0", "sai_test-sai_impl-1.5.0"]),
         "gflags": (LIB, ["libgflags.so.2.2"]),
         "glog": (LIB64, ["libglog.so.0"]),
         "zstd": (LIB64, ["libzstd.so.1.3.8"]),
diff --git a/installer/centos-7-x86_64/fboss_bins-1.spec b/installer/centos-7-x86_64/fboss_bins-1.spec
index 2a451d5..bc647f7 100644
--- a/installer/centos-7-x86_64/fboss_bins-1.spec
+++ b/installer/centos-7-x86_64/fboss_bins-1.spec
@@ -22,6 +22,7 @@ install -m 0755 -d $RPM_BUILD_ROOT/opt/fboss
 # install binaries
 install -m 0755 wedge_agent %{buildroot}/opt/fboss/wedge_agent
 install -m 0755 bcm_test %{buildroot}/opt/fboss/bcm_test
 install -m 0755 sai_test-fake-1.5.0 %{buildroot}/opt/fboss/sai_test-fake-1.5.0
+install -m 0755 sai_test-fake-1.5.0 %{buildroot}/opt/fboss/sai_test-sai_impl-1.5.0

 #install dependent libraries
 install -m 0755 libgflags.so.2.2 %{buildroot}/opt/fboss/libgflags.so.2.2
@@ -37,6 +38,7 @@ install -m 0755 libnghttp2.so.14 %{buildroot}/opt/fboss/libnghttp2.so.14
 %files
 /opt/fboss/wedge_agent
 /opt/fboss/bcm_test
 /opt/fboss/sai_test-fake-1.5.0
+/opt/fboss/sai_test-sai_impl-1.5.0

 /opt/fboss/libgflags.so.2.2
 /opt/fboss/libglog.so.0
```

---
# Validating FBOSS build with a new OpenNSA SDK

Before releasing the next version of OpenNSA on github, the following steps could be
used to validate the FBOSS build with the new OpenNSA

- Copy over ```OpenNSA-next-release.tar.gz``` on to a FBOSS build VM
- From the same directory, start local web server: ```python3 -m http.server &```
- Edit ```fboss.git/build/fbcode_builder/manifests/OpenNSA``` to update url,
  sha256 sum, subdir etc.

    ```
    diff --git a/build/fbcode_builder/manifests/OpenNSA b/buildfbcode_builder/manifests/OpenNSA
    --- a/build/fbcode_builder/manifests/OpenNSA
    +++ b/build/fbcode_builder/manifests/OpenNSA
    @@ -2,15 +2,15 @@
     name = OpenNSA

     [download]
    -url = https://bitbucket.org/fboss/opennsa-fork/get/release/v1.0.tar.gz
    -sha256 = 40e56460b85a8be4cfdc4591569453eb19aea3344f3c297b1d8b5a9ebf29bdf0
    +url = http://localhost:8000/OpenNSA-next-release.tar.gz
    +sha256 = <insert the output of 'sha256sum OpenNSA-next-release.tar.gz' here>

     [build]
     builder = nop
    -subdir = fboss-opennsa-fork-1e7cd6e7f059
    +subdir = fboss-opennsa-fork-26d1e23b9867 # Replace with appropriate subdir name>
    ```

- Build OpenNSA. Since OpenNSA has precompiled libraries, this does not do a
  real build, but simply copies over the files to the right directories for
  fbcode_builder to consume while building FBOSS.

    ```
    export CPLUS_INCLUDE_PATH=/opt/rh/rh-python36/root/usr/include/python3.6m/
    export PATH=/opt/rh/devtoolset-8/root/usr/bin/:/opt/rh/rh-python36/root/usr/bin:$PATH
    cd fboss.git
    ./build/fbcode_builder/getdeps.py build OpenNSA
    ./build/fbcode_builder/getdeps.py show-inst-dir OpenNSA
    ls <output of show-inst-dir> to verify if the OpenNSA-next-release libs are available.
    ```

- [Rebuild FBOSS](#build-fboss) with OpenNSA-next-release after cleaning dependencies
```
./build/fbcode_builder/getdeps.py clean
```
If the build succeeds, it validates that the next OpenNSA release can build with FBOSS.

---
# Appendix A

This section details workflows using particular versions of software. The
build/install process has been successfully verified using these workflows and
should thus serve as handy reference.

<a name="fusion-install">

## A.1 Installing VM on VMware Fusion

</a>

These steps worked on a Macbook laptop running macOS Mojave version 10.14.4.

- Install VMware Fusion Professional Version 11.1.0 (13668589)
- Download CentOS-7-x86_64-DVD-1810.iso https://www.centos.org/download
- Start VMware Fusion : File => New => drag and drop CentOS ISO on
   ```Install from disc or image```.
- On macOS Mojave Version 10.14.4, this prompts ```Could not open
  /dev/vmmon: Broken Pipe```. Fix by setting right security & privacy
  settings. Reference: http://communities.vmware.com/thread/600496
- Before beginning installation, click on VM settings and change
  hard disk size to at least 64G, CPUs: 2 (max the machine can reasonably
  support), Memory 2G (again, max possible).

<a name="usb-centos-install">

## A.2 Installing CentOS 7 from bootable USB

</a>

These steps worked on a Macbook laptop running macOS Mojave version 10.14.4.
These steps require a USB 2.0 or USB 3.0 of size >= 16GB.

### A.2.1 Create a bootable USB

- Download ISO: CentOS 7 x86_64 from: https://www.centos.org/download
- Convert downloaded .iso to .img (specifically, a UDIF read/write image) by
  running following from Macbook terminal

    ```
    hdiutil convert -format UDRW -o CentOS-7-x86_64-DVD-1810.img CentOS-7-x86_64-DVD-1810.iso
    ```

- Insert USB 2.0 or USB 3.0 drive into the macbook.
- Find out the name of the USB disk just inserted e.g.  /dev/disk3
  ```
  diskutil list
  ```
- Unmount the disk e.g. ```diskutil unmountDisk /dev/disk3```
  ```
  diskutil unmountDisk <disk-name>
  ```
- This next step will take a **long** time without reporting any progress. _Took over 25 minutes on Macbook laptop with Interl i7 processor and 16G RAM._
  ```
  time sudo dd if=CentOS-7-x86_64-DVD-1810.img.dmg of=/dev/disk3 bs=1m
  ```

Reference: http://www.myiphoneadventure.com/os-x/create-a-bootable-centos-usb-drive-with-a-mac-os-x

### A.2.2 Use the bootable USB to install CentOS on the Wedge switch

- Power on the switch and connect to its serial console (BMC).
- Insert the bootable USB.
- Login as root using the default password.
- From the command prompt, run ```/usr/local/bin/wedge_power.sh reset```
- Then, run ```/usr/local/bin/sol.sh``` to get serial console to the wedge to
  observe the wedge restart.
- During restart, hit ```escape``` or ```delete``` or ```shift delete``` to get to
  the BIOS. Depending on the environment used to connect to console, one of the above
  key combinations should work.
- From BIOS, select to boot from USB.
  (note: if the configuration order is changed to boot from USB, don't forget
  to change it back to boot from hard disk after the install is complete.
  Alternatively, simply choose to boot from USB for the current boot).
- This will boot from USB and get to the Linux grub prompt.
- By default, Linux will attempt GUI install, to prevent it, at grub prompt:
    - edit line that has ```quite``` in the end to add ```inst.text inst.headless console=ttyS0,57600```
    - hit ```CTRL + x``` to boot using above setting, this will do text install
    from the console.
      - Set the timezone, root password
      - Choose defaults for destination disk: ```Use all of space```, ```LVM```.
    - **NOTE**: post install step takes several minutes.
    - ```reboot```
- If the boot order was changed, enter BIOS again to change it back to boot
  from hard disk first.

### A.2.3 Configure Network

This assumes DHCP server is present in the network.

- From the linux prompt, run ```ip addr list``` to list interfaces.
- Request DHCP address on interface. e.g. ```dhclient <interface-name>``` or
  ```dhclient -6 <interface-name>``` (for IPv6)
- ```dhclient``` step has to be run after every reboot, to make it persistent:
  - Edit ```/etc/sysconfig/network-scripts/ifcfg-<interface-name>``` e.g. for ```/etc/sysconfig/network-scripts/ifcfg-enp2s0``` if the interface name
    is ```enp2s0```
  - Set ```ONBOOT=yes```
