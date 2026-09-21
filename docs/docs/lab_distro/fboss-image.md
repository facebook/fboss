---
id: fboss_image_cli
title: fboss-image tool
description: How to use the fboss-image tool
keywords:
    - FBOSS
    - OSS
    - build
oncall: fboss_oss
---

# fboss-image tool

## Quick Start

Requires Python 3.10+. No external dependencies.

```bash
# Build an image
./fboss-image build manifest.json

# Build specific components
./fboss-image build manifest.json kernel sai

# Device management
./fboss-image device <mac-address> image-upstream <kernel-version> <sai>
./fboss-image device <mac-address> image <image.bin>
./fboss-image device <mac-address> reprovision
./fboss-image device <mac-address> update manifest.json <component>
./fboss-image device <mac-address> getip
./fboss-image device <mac-address> ssh
```

## Introduction

The `fboss-image` CLI tool is a tool for building FBOSS Lab Distribution images and managing devices in combination with
the lab quickstart Distro Infra Container.

## fboss-image build

The 'build' command subtree handles building FBOSS Distro Image components and the full image itself. The syntax is:
```
fboss-image build <manifest path> [<component1> [<component2> ...]]
```
If no components are specified, all components of the image will be built and all the image types configured in the
manifest will be produced.  If components are specified, only those components will be rebuilt and the complete image(s)
will not be produced.

The manifest file is described in [Distro Image Manifest format](/docs/lab_distro/distro_image_manifest).

## fboss-image device

The 'device' command subtree deals with getting FBOSS Distro images onto devices.

In order to support PXE boot without taking full control of the network DHCP server and support varying DHCP client IDs,
it is necessary to identify FBOSS devices by the Ethernet MAC address of its management port. The Distro Infrastructure
container will attempt to passively maintain a mapping from IP-to-MAC, perhaps from TFTP or HTTP file accesses or its
local neighbor table. If this mapping is not available the Distro CLI will attempt to scan for it, but that will only
work if the workstation is L2 connected to the management port of the device.

The 'device' command further has several sub-commands:

image-upstream: `fboss-image device <MAC> image-upstream <image description>`

This command fetches a reference image from Meta and configures the Distro Infrastructure to serve this image to the
device.

image: `fboss-image device <MAC> image <image file>`

This command configures the Distro Infrastructure to serve the given image to the device. PXE versus ONIE installer is
detected.

reprovision: `fboss-image device <MAC> reprovision`

This command attempts to log onto the device and wipe FBOSS to a factory-fresh state such that the device will perform a
netboot. This command will then attempt to reboot the device to trigger this netboot.

update: `fboss-image device <MAC> update <manifest file> <component>`

This command invokes the build for, and then attempts to login and update, the given component on the device. Once
copied to the device the relevant service(s) will be restarted. Not all components are supported, such as the kernel.

getip: `fboss-image device <MAC> getip`

The getip command attempts to retrieve the IP for the given device.

ssh: `fboss-image device <MAC> ssh`

The ssh command is a convenience helper which is equivalent to `ssh admin@$(fboss-image device <MAC> getip)`.
