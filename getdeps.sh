#!/usr/bin/env bash

function die() {
    echo "$1 : fatal error; dying..." >&2
    exit 1
}

function update() {
    repo=`basename $1 .git`
    echo "updating $repo..."
    if [ -d $repo ]; then
        (cd $repo && git pull)
    else
        git clone "$1" || die "Failed to clone $1"
        [ -z "$2" ] || (cd $repo && git checkout $2)
    fi
}

function update_branch() {
    branch="$1"
    if [ "$(git symbolic-ref -q --short HEAD)" != "OpenNSL_6.3" ]; then
        git checkout -tb "$branch" "origin/$branch"
    fi
}

function build_autoconf() {
    (
    echo "building $1..."
    cd "$1" || die "Missing dir $1"
    if [ ! -e ./configure ]; then
      autoreconf --install
    fi
    ./configure || die "Configure failed for $1"
    make -j "$NPROC" || die "Make failed for $1"
    # do NOT quote "\$2" here because it passes an empty filename
    # to make as a target
    make install prefix="$INSTALL_DIR" $2 || \
        die "Failed to install $1 to $INSTALL_DIR"
    )
}

function build_make() {
    (
    echo "Building $1..."
    cd "$1" || die "Missing dir $1"
    if [ ! -e Makefile ] ; then
       die "Missing Makefile for $1"
    fi
    make -j "$NPROC"|| die "Make failed for $1"
    make install prefix="$INSTALL_DIR" || \
        die "Failed to install $1 to $INSTALL_DIR"
    )
}

function build_cmake() {
    (
        echo "building $1..."
        cd "$1" || die "Missing dir $1"
        if [ -e ./CMakeLists.txt ]; then
            mkdir -p _build || true
            cd _build || die "Failed to cd into _build directory"
            cmake configure .. \
                -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
                -DCMAKE_PREFIX_PATH="$INSTALL_DIR" \
                -DBUILD_TESTS=OFF \
                || die "Cmake failed for $1"
            make -j "$NPROC" || die "Failed to build $1"
            make install || die "Failed to install $1 to $INSTALL_DIR"
        else
            die "No CmakeLists.txt found for $1"
        fi
    )
}

function get_packages() {
    echo "updating package indices"
    sudo apt-get update
    echo "installing packages"
    sudo apt-get install -yq autoconf automake libdouble-conversion-dev \
        libssl-dev make zip git autoconf libtool g++ libboost-all-dev \
        libevent-dev flex bison libgoogle-glog-dev scons libkrb5-dev \
        libsnappy-dev libsasl2-dev libnuma-dev libi2c-dev libcurl4-nss-dev \
        libusb-1.0-0-dev libpcap-dev libdb5.3-dev cmake libnl-3-dev \
        libnl-route-3-dev iptables-dev

   sudo apt-get install -yq shtool pkg-config
}

if [ "$1" != 'nopkg' ]; then
    get_packages
else
  shift
fi

if [ "$1" == 'pkgsonly' ]; then
    echo Requested to install packages only >&2
    exit 0
fi

GET_OPENNSL=1

if [ "$1" == "nobcm" ]; then
  GET_OPENNSL=0   # used by people concerned with OpenNSL agreement
                  # WARNING: setting this will currently break compile!
fi

echo "creating external..."
mkdir -p external
NPROC=$(grep -c processor /proc/cpuinfo)
(
    cd external
    EXT=$(pwd)
    # build and install everything to this dir
    INSTALL_DIR="$EXT.install"
    for p in bin sbin lib include; do mkdir -p "$INSTALL_DIR/$p" ; done
    if [ $GET_OPENNSL == 1 ] ; then
      # We hard code OpenNSL to OpenNSL-6.4.6.6 release, later releases seem to
      # SIGSEV in opennsl_pkt_alloc()
      update https://github.com/Broadcom-Switch/OpenNSL.git 8e0b499f02dcef751a3703c9a18600901374b28a
      # Opennsl-3.5.0.1
      #update https://github.com/Broadcom-Switch/OpenNSL.git 0478ce76b0430f079b3c7addc821cd56f26c2cab
    fi
    # iproute2 v4.4.0
    update https://git.kernel.org/pub/scm/linux/kernel/git/shemminger/iproute2.git 7ca63aef7d1b0c808da0040c6b366ef7a61f38c1
    update https://github.com/facebook/folly.git
    update https://github.com/jedisct1/libsodium.git stable
    update https://github.com/facebookincubator/fizz.git
    update https://github.com/facebook/wangle.git
    update https://github.com/facebook/fbthrift.git
    update https://github.com/no1msd/mstch.git
    update https://github.com/facebook/zstd.git master
    update https://github.com/google/googletest.git release-1.8.0
    build_cmake mstch || die "Failed to build mstch"
    build_make zstd || die "Failed to build zstd"
    build_autoconf iproute2 "DESTDIR=$INSTALL_DIR" || die "Failed to build iproute2"
    build_cmake folly || die "Failed to build folly"
    build_autoconf libsodium || die "Failed to build sodium"
    build_cmake fizz/fizz || die "Failed to build fizz"
    build_cmake wangle/wangle || die "Failed to build wangle"
    build_cmake fbthrift/ || die "Failed to build thrift"
)
