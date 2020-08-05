#!/usr/bin/env bash

sudo yum install -y git gcc gcc-c++ python-devel rh-python36 openssl*
sudo yum install -y centos-release-scl-rh devtoolset-8-toolchain
sudo yum group install "Development Tools" -y
sudo yum install rpm-build rpmdevtools -y
sudo yum --enablerepo=extras install epel-release
sudo yum install python-pip -y
sudo pip install gitpython
sudo yum install devtoolset-8-libasan-devel devtoolset-8-libubsan-devel
