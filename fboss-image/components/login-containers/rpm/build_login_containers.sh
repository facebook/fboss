#!/bin/bash
# Build the login-containers RPM from ../payload. Runs in the builder container
# via the manifest's other_dependencies execute entry; also runnable by hand.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
PAYLOAD="$SCRIPT_DIR/../payload"
[ -d "$PAYLOAD" ] || { echo "payload dir missing: $PAYLOAD" >&2; exit 1; }
rpmdir=$(pwd)/rpmbuild
mkdir -p "${rpmdir}"/{SOURCES,SPECS,RPMS,SRPMS,BUILD,BUILDROOT}
cp "$SCRIPT_DIR/login-containers.spec" "${rpmdir}/SPECS/"
rpmbuild -bb --define "_topdir ${rpmdir}" --define "_payload ${PAYLOAD}" \
  "${rpmdir}/SPECS/login-containers.spec"
mkdir -p "${SCRIPT_DIR}/dist"
cp "${rpmdir}"/RPMS/noarch/login-containers-*.rpm "${SCRIPT_DIR}/dist/"
rm -rf "${rpmdir}"
