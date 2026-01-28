Distro Infrastructure
=====================

This directory contains the FBOSS Distro Infracture container. Right now it provides only the necessary support for IPv4
and IPv6 PXE boot. Under IPv4 the network must have a DHCP server providing IP addresses. Under IPv6 the network must
**NOT** have a DHCP server providing IP addresses.

Building
--------

Build the container with `./build.sh`.

Usage
-----

Start the container by running the 'distro_infra.sh' script which will start the container. This script takes two
arguments:

- interface: The interface to attach to. This must have L2 adjacency with the management port of the FBOSS duts
- persistent directory: The directory to use for persistent storage, primarily of images to load

For example, `mkdir images; ./distro_infra.sh vlan1033 images`.

This will start an interactive tool to configure supplying PXE boot options to a given MAC address. Exiting the tool
terminates the container.

The first time a MAC address is given, a `<MAC>` directory under the persistent directory will be created. Into this the
image files to boot must be manually extracted. eg:

```
$ ./distro_infra.sh vlan1033 images
Listening on vlan1033 - 10.250.33.194
dnsmasq: started, version 2.85 DNS disabled
dnsmasq: compile time options: IPv6 GNU-getopt DBus no-UBus no-i18n IDN2 DHCP DHCPv6 no-Lua TFTP no-conntrack ipset auth
cryptohash DNSSEC loop-detect inotify dumpfile
dnsmasq-dhcp: DHCP, proxy on subnet 10.250.33.194
...
Enter MAC address (blank to exit): DC-DA-4D-FC-AD-2D
```

Now images/dc-da-4d-fc-ad-2d has been created. Into that directory the following files must be copied (or hardlinked
from the also created `images/cache` directory), or extracted from the PXE installer tarball, which contains these
precise names:

```
$ cd images/dc-da-4d-fc-ad-2d
$ tar -xf fboss-distro-image_pxe.tar
$ ls -1
FBOSS-Distro-Image.x86_64-1.0.config.bootoptions
FBOSS-Distro-Image.x86_64-1.0.initrd
FBOSS-Distro-Image.x86_64-1.0.kernel
FBOSS-Distro-Image.x86_64-1.0.sha256
FBOSS-Distro-Image.x86_64-1.0.xz
pxeboot.FBOSS-Distro-Image.x86_64-1.0.kernel
pxeboot.FBOSS-Distro-Image.x86_64-1.0.initrd
```

Other files will be generated and populated automatically.

The dut can then be rebooted and PXE boot will start. Once PXE boot has completed, the Distro Infrastructure container
will stop serving PXE boot to that MAC. To serve PXE boot again, re-enter the MAC address at the menu. It is not
necessary to recopy the image files.

To terminate the script and container, enter a blank MAC address at the prompt: `Enter MAC address (blank to exit):`.
