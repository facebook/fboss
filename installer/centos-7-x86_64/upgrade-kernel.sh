#!/usr/bin/env bash

# Kernel version for FBOSS build
LTKERNELVER=4.4.219-1.el7.elrepo.x86_64

sudo rpm --import https://www.elrepo.org/RPM-GPG-KEY-elrepo.org
sudo rpm -Uvh https://elrepo.org/linux/kernel/el7/x86_64/RPMS/kernel-lt-$LTKERNELVER.rpm
sudo rpm -Uvh https://elrepo.org/linux/kernel/el7/x86_64/RPMS/kernel-lt-devel-$LTKERNELVER.rpm
