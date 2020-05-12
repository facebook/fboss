#!/usr/bin/env bash

# Kernel version for FBOSS build
LTKERNELVER=4.4.219-1.el7.elrepo.x86_64

# Download kernel RPMs from one of the archives @ https://elrepo.org/tiki/Download
sudo rpm --import https://www.elrepo.org/RPM-GPG-KEY-elrepo.org
sudo rpm -Uvh http://mirror.pit.teraswitch.com/elrepo/kernel/el7/x86_64/RPMS/kernel-lt-$LTKERNELVER.rpm
sudo rpm -Uvh http://mirror.pit.teraswitch.com/elrepo/kernel/el7/x86_64/RPMS/kernel-lt-devel-$LTKERNELVER.rpm
