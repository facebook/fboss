#!/bin/bash
set -e

dnf builddep -y --spec rpmbuild/SPECS/demo_container.spec

rpmdir=$(pwd)/rpmbuild
mkdir -p ${rpmdir}/{RPMS,SRPMS,BUILD,BUILDROOT}

rpmbuild -bb --define "_topdir ${rpmdir}" ${rpmdir}/SPECS/demo_container.spec

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
mkdir -p ${SCRIPT_DIR}/dist
cp ${rpmdir}/RPMS/*/*.rpm ${SCRIPT_DIR}/dist
