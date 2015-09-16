#!/usr/bin/env bash

function update() {
    repo=`basename $1 .git`
    echo "updating $repo..."
    if [ -d $repo ]; then
        (cd $repo && git pull)
    else
        git clone $1
        git checkout $2
    fi
}

function update_branch() {
    branch="$1"
    if [ "$(git symbolic-ref -q --short HEAD)" != "OpenNSL_6.3" ]; then
        git checkout -tb "$branch" "origin/$branch"
    fi
}

function build() {
    (
        echo "building $1..."
        cd $1
        if [ ! -x ./configure ]; then
            autoreconf --install
            ./configure
        fi
        make -j8
    )
}

echo "installing packages"
sudo apt-get install -yq autoconf automake libdouble-conversion-dev \
    libssl-dev make zip git autoconf libtool g++ libboost-all-dev \
    libevent-dev flex bison libgoogle-glog-dev scons libkrb5-dev \
    libsnappy-dev libsasl2-dev libnuma-dev libi2c-dev libcurl4-nss-dev

echo "creating external..."
mkdir -p external
(
    cd external
    update https://github.com/Broadcom-Switch/OpenNSL.git
    (cd OpenNSL && update_branch OpenNSL_6.3)
    update \
        git://git.kernel.org/pub/scm/linux/kernel/git/shemminger/iproute2.git
    update https://github.com/facebook/folly.git
    update https://github.com/facebook/fbthrift.git
    build iproute2 v3.12.0
    build folly/folly v0.48.0
    export CPPFLAGS=" -I`pwd`/folly/" LDFLAGS="-L`pwd`/folly/folly/.libs/"
    build fbthrift/thrift v0.28.0
)
