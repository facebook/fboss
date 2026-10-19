Name: login-containers
Version: 1.0
Release: 1
Summary: Role login containers (nspawn machines for netops/netadmin)
License: GPL
BuildArch: noarch
%description
nspawn login machines for the netops/netadmin roles: machine rootfs skeletons,
nspawn settings, machine units, login-shell wrappers, and the sudoers grant.
Shell assignment happens in the manifest execute hook (users exist only
after the after_pkgs hooks run).
%install
cp -a %{_payload}/* %{buildroot}/
# Build-system shims (BUCK export_files for tests) live next to the scripts
# they export but must not ship in the RPM: drop them from the buildroot.
find %{buildroot} -name BUCK -delete
# Skeleton dirs: empty dirs are not tracked in the repo, so create them here.
# (%files owns them with %dir; home dirs get numeric ownership via %attr.)
for m in netops netadmin; do
  mkdir -p "%{buildroot}/var/lib/machines/$m"/{home,root,tmp,var,run}
done
mkdir -p "%{buildroot}/var/lib/machines/netops/home/netops"
mkdir -p "%{buildroot}/var/lib/machines/netadmin/home/netadmin"
%post
# %post does not run under set -e: guard enable, then verify the state.
# An un-enabled unit is exactly the dead-login image this spec refuses
# to ship, and nothing below inspects enablement.
systemctl enable netops-machine.service netadmin-machine.service \
  || { echo "login-containers: systemctl enable failed" >&2; exit 1; }
systemctl is-enabled --quiet netops-machine.service netadmin-machine.service \
  || { echo "login-containers: units not enabled" >&2; exit 1; }
for sh in /usr/local/bin/netops-shell /usr/local/bin/netadmin-shell; do
  grep -qxF "$sh" /etc/shells || echo "$sh" >> /etc/shells
done
# Machine homes get numeric ownership: %attr user/group must be names, and the
# role users do not exist at RPM install time (created later by the hook).
# chown needs no NSS lookup, so numerics work here.
chown 1000:1000 /var/lib/machines/netops/home/netops
chmod 700 /var/lib/machines/netops/home/netops
chown 1001:1002 /var/lib/machines/netadmin/home/netadmin
chmod 700 /var/lib/machines/netadmin/home/netadmin
# Every file the login path requires. Fail the install (and the image
# build) rather than ship an image whose logins break at first use.
for f in /var/lib/machines/netops/etc/passwd \
    /var/lib/machines/netops/etc/shadow \
    /var/lib/machines/netops/etc/machine-id \
    /var/lib/machines/netops/etc/os-release \
    /var/lib/machines/netops/etc/systemd/system/login-shell.target \
    /var/lib/machines/netadmin/etc/passwd \
    /var/lib/machines/netadmin/etc/shadow \
    /var/lib/machines/netadmin/etc/machine-id \
    /var/lib/machines/netadmin/etc/os-release \
    /var/lib/machines/netadmin/etc/systemd/system/login-shell.target \
    /var/lib/machines/netadmin/etc/sudoers \
    /var/lib/machines/netadmin/etc/sysusers.d/netadmin.conf \
    /var/lib/machines/netadmin/etc/pam.d/sudo \
    /etc/systemd/nspawn/netops.nspawn \
    /etc/systemd/nspawn/netadmin.nspawn \
    /usr/lib/systemd/system/netops-machine.service \
    /usr/lib/systemd/system/netadmin-machine.service \
    /usr/local/bin/netops-shell \
    /usr/local/bin/netadmin-shell \
    /usr/local/bin/role-entry \
    /usr/local/bin/session-recorder \
    /usr/local/bin/session-ancestors \
    /etc/sudoers.d/login-containers; do
  [ -s "$f" ] || { echo "login-containers: missing $f" >&2; exit 1; }
done
[ "$(stat -c %a /etc/sudoers.d/login-containers)" = "440" ] || { echo "login-containers: bad sudoers mode" >&2; exit 1; }
visudo -c -f /etc/sudoers.d/login-containers || { echo "login-containers: bad sudoers syntax" >&2; exit 1; }
[ "$(stat -c %a /var/lib/machines/netadmin/etc/sudoers)" = "440" ] || { echo "login-containers: bad machine sudoers mode" >&2; exit 1; }
visudo -c -f /var/lib/machines/netadmin/etc/sudoers || { echo "login-containers: bad machine sudoers syntax" >&2; exit 1; }
for d in /var/lib/machines/netops/home/netops \
    /var/lib/machines/netadmin/home/netadmin; do
  [ -d "$d" ] || { echo "login-containers: missing $d" >&2; exit 1; }
done
[ "$(stat -c %u:%g:%a /var/lib/machines/netops/home/netops)" = "1000:1000:700" ] || { echo "login-containers: bad netops home ownership" >&2; exit 1; }
[ "$(stat -c %u:%g:%a /var/lib/machines/netadmin/home/netadmin)" = "1001:1002:700" ] || { echo "login-containers: bad netadmin home ownership" >&2; exit 1; }
%files
%defattr(-,root,root,-)
%attr(0755,root,root) /usr/local/bin/netops-shell
%attr(0755,root,root) /usr/local/bin/netadmin-shell
%attr(0755,root,root) /usr/local/bin/role-entry
%attr(0755,root,root) /usr/local/bin/session-recorder
%attr(0755,root,root) /usr/local/bin/session-ancestors
%attr(0440,root,root) /etc/sudoers.d/login-containers
/etc/systemd/nspawn/netops.nspawn
/etc/systemd/nspawn/netadmin.nspawn
/usr/lib/systemd/system/netops-machine.service
/usr/lib/systemd/system/netadmin-machine.service
/var/lib/machines/netops/etc/passwd
/var/lib/machines/netops/etc/group
/var/lib/machines/netops/etc/shadow
/var/lib/machines/netops/etc/hostname
/var/lib/machines/netops/etc/machine-id
/var/lib/machines/netops/etc/os-release
/var/lib/machines/netops/etc/systemd/system/login-shell.target
/var/lib/machines/netops/etc/systemd/system/dbus.service
/var/lib/machines/netops/etc/systemd/system/default.target
/var/lib/machines/netops/bin
/var/lib/machines/netops/sbin
/var/lib/machines/netops/lib
/var/lib/machines/netops/lib64
%dir /var/lib/machines/netops/home/netops
%dir /var/lib/machines/netops/home
%dir /var/lib/machines/netops/root
%dir /var/lib/machines/netops/tmp
%dir /var/lib/machines/netops/var
%dir /var/lib/machines/netops/run
/var/lib/machines/netadmin/etc/passwd
/var/lib/machines/netadmin/etc/group
/var/lib/machines/netadmin/etc/shadow
/var/lib/machines/netadmin/etc/hostname
/var/lib/machines/netadmin/etc/machine-id
/var/lib/machines/netadmin/etc/os-release
%attr(0440,root,root) /var/lib/machines/netadmin/etc/sudoers
/var/lib/machines/netadmin/etc/sysusers.d/netadmin.conf
/var/lib/machines/netadmin/etc/pam.d/sudo
/var/lib/machines/netadmin/etc/systemd/system/login-shell.target
/var/lib/machines/netadmin/etc/systemd/system/dbus.service
/var/lib/machines/netadmin/etc/systemd/system/default.target
/var/lib/machines/netadmin/bin
/var/lib/machines/netadmin/sbin
/var/lib/machines/netadmin/lib
/var/lib/machines/netadmin/lib64
%dir /var/lib/machines/netadmin/home/netadmin
%dir /var/lib/machines/netadmin/home
%dir /var/lib/machines/netadmin/root
%dir /var/lib/machines/netadmin/tmp
%dir /var/lib/machines/netadmin/var
%dir /var/lib/machines/netadmin/run
