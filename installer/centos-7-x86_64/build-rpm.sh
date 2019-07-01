#!/usr/bin/env bash

SCRIPT=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT")

FBOSS_BINS=fboss_bins-1
RPM_SPEC_NAME="$FBOSS_BINS".spec
RPM_SPEC=$SCRIPT_DIR/$RPM_SPEC_NAME

FBOSS_BIN_DIR=/tmp/fbcode_builder_getdeps-ZhomeZ${USER}Zfboss.gitZbuildZfbcode_builder/installed
RPM_DIR=$HOME/rpmbuild
RPM_SOURCES_DIR=$RPM_DIR/SOURCES
RPM_SOURCES_BINS_DIR=$RPM_SOURCES_DIR/$FBOSS_BINS
RPM_SPECS_DIR=$RPM_DIR/SPECS

DEVTOOLS_LIBRARY_PATH=/opt/rh/devtoolset-8/root/usr/lib64

function die() {
    echo "$1 : fatal error; dying..." >&2
    exit 1
}

function setup_for_rpmbuild() {
  echo "Setup for rpmbuild ..."
  # start afresh, create RPM build tree within user's home directory
  rm -rf "$RPM_DIR"
  rpmdev-setuptree
}

function copy_binaries() {
  echo "Copy FBOSS bins built by fbcode_builder for rpmbuild"

  mkdir "$RPM_SOURCES_BINS_DIR"

  # copy binaries
  cp "$FBOSS_BIN_DIR"/fboss/bin/* "$RPM_SOURCES_BINS_DIR"

  # copy dependent libraries
  cp "$FBOSS_BIN_DIR"/gflags-*/lib/libgflags.so.2.2 "$RPM_SOURCES_BINS_DIR"
  cp "$FBOSS_BIN_DIR"/glog-*/lib64/libglog.so.0 "$RPM_SOURCES_BINS_DIR"
  cp "$FBOSS_BIN_DIR"/zstd-*/lib64/libzstd.so.1.3.8 "$RPM_SOURCES_BINS_DIR"
  cp "$FBOSS_BIN_DIR"/OpenNSL-*/lib/libopennsl.so.1 "$RPM_SOURCES_BINS_DIR"
  cp "$FBOSS_BIN_DIR"/libusb-*/lib/libusb-1.0.so.0 "$RPM_SOURCES_BINS_DIR"
  cp "$FBOSS_BIN_DIR"/libnl-*/lib/libnl-3.so.200 "$RPM_SOURCES_BINS_DIR"
  cp "$FBOSS_BIN_DIR"/libcurl-*/lib/libcurl.so.4 "$RPM_SOURCES_BINS_DIR"
  cp "$FBOSS_BIN_DIR"/libsodium-*/lib/libsodium.so.23 "$RPM_SOURCES_BINS_DIR"
  cp "$FBOSS_BIN_DIR"/libmnl-*/lib/libmnl.so.0 "$RPM_SOURCES_BINS_DIR"
}

function prepare_for_build() {
  echo "Preparing for RPM build..."

  # Prepare to build RPM
  cd "$RPM_SOURCES_DIR" || die "Missing dir $RPM_SOURCES_DIR"
  tar czf "$FBOSS_BINS".tar.gz "$FBOSS_BINS"
  rm -rf "$FBOSS_BINS" # we will package tar.gz, so we can remove

  # TODO use rpmdev-newspec $FBOSS_BINS to create spec and edit it.
  # For now, copy pre-created SPEC file.

  cp "$RPM_SPEC" "$RPM_SPECS_DIR"
}

function build_rpm() {
  echo "Build RPM... "

  cd "$RPM_DIR" || die "Missing dir $RPM_DIR"
  LD_LIBRARY_PATH="$DEVTOOLS_LIBRARY_PATH" rpmbuild -ba "$RPM_SPECS_DIR"/"$RPM_SPEC_NAME"
}

setup_for_rpmbuild
copy_binaries
prepare_for_build
build_rpm
