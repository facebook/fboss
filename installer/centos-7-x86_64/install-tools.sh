#!/usr/bin/env bash

sudo yum install -y git gcc g++ python-devel rh-python36 openssl*
sudo yum install -y centos-release-scl-rh devtoolset-8-toolchain
sudo yum group install "Development Tools"
sudo yum install rpm-build rpmdevtools
