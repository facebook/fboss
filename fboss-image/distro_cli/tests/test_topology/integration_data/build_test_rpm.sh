#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RPMDIR=$(mktemp -d)
trap 'rm -rf "${RPMDIR}"' EXIT

mkdir -p "${RPMDIR}"/{SOURCES,SPECS}

cat >"${RPMDIR}/SOURCES/test-tool" <<'EOF'
#!/bin/bash
echo "test-tool 2.0.0"
EOF
chmod +x "${RPMDIR}/SOURCES/test-tool"

cat >"${RPMDIR}/SPECS/test-tool.spec" <<'EOF'
Name: test-tool
Version: 2.0.0
Release: 1
BuildArch: noarch
%install
mkdir -p %{buildroot}/usr/local/bin
install -m 755 %{_sourcedir}/test-tool %{buildroot}/usr/local/bin/test-tool
%files
/usr/local/bin/test-tool
EOF

rpmbuild -bb --quiet --define "_topdir ${RPMDIR}" "${RPMDIR}/SPECS/test-tool.spec"
cp "${RPMDIR}/RPMS/noarch/test-tool-2.0.0-1.noarch.rpm" "${SCRIPT_DIR}/"
