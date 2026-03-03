#!/bin/bash

# Sample thirdparty build script

echo "Executing --- ${BASH_SOURCE[0]} $* ----"

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"

# Perform thirdparty build here ----

# Generate the artifact rpm package in dist directory
rpmdir=$(pwd)/rpmbuild
mkdir -p ${rpmdir}/{SOURCES,SPECS,RPMS,SRPMS,BUILD,BUILDROOT}

# Create the blank spec file
cat <<EOF >${rpmdir}/SPECS/thirdparty.spec
Name: thirdparty-pkg
Version: 1.0
Release: 1
Summary: Empty
License: GPL
BuildArch: noarch
%description
Empty
%files
EOF

# Build the rpm
rpmbuild -bb --define "_topdir ${rpmdir}" ${rpmdir}/SPECS/thirdparty.spec

# Copy the rpm to dist directory
mkdir -p ${SCRIPT_DIR}/dist
cp ${rpmdir}/RPMS/noarch/thirdparty-pkg-1.0-1.noarch.rpm ${SCRIPT_DIR}/dist

# Cleanup - remove the rpmbuild directory
rm -rf ${rpmdir}
exit 0
