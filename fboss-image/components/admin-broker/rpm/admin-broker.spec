Name: admin-broker
Version: 1.0
Release: 1
Summary: Scoped host control channel for role login containers
License: GPL
BuildArch: noarch
%description
Host-side broker for the netops/netadmin login containers: an abstract-socket
daemon that attributes requests by kernel peer credentials plus cgroup session
scope, audits every action, and exposes allowlisted host operations
(identity/process relay in B1; unit control and namespace entry follow).
%install
cp -a %{_payload}/* %{buildroot}/
# Build-system shims (BUCK export_files for tests) live next to the scripts
# they export but must not ship in the RPM: drop them from the buildroot.
find %{buildroot} -name BUCK -delete
%post
# %post does not run under set -e: guard enable, then verify the state.
systemctl enable admin-broker.service \
  || { echo "admin-broker: systemctl enable failed" >&2; exit 1; }
systemctl is-enabled --quiet admin-broker.service \
  || { echo "admin-broker: unit not enabled" >&2; exit 1; }
# Fail the install (and the image build) rather than ship an image whose
# broker is missing or not executable.
[ -x /usr/local/sbin/admin-broker ] || { echo "admin-broker: missing daemon" >&2; exit 1; }
[ -s /usr/lib/systemd/system/admin-broker.service ] || { echo "admin-broker: missing unit" >&2; exit 1; }
[ -s /etc/admin-broker/allowlist.conf ] || { echo "admin-broker: missing allowlist" >&2; exit 1; }
/usr/local/sbin/admin-broker --help >/dev/null || { echo "admin-broker: daemon does not start" >&2; exit 1; }
%files
%defattr(-,root,root,-)
%attr(0755,root,root) /usr/local/sbin/admin-broker
/usr/lib/systemd/system/admin-broker.service
/etc/admin-broker/allowlist.conf
