Name: fake-ident
Version: 1.0
Release: 1
Summary: Fake-distro identity files (VM/dev only, never for hardware)
License: GPL
BuildArch: noarch
%description
Synthetic platform identity for the fake-SAI distro VM: fruid, agent seed,
platform_manager config, and service drop-ins. Substitutes for the hardware
(EEPROM) and platform packages a real switch provides.
%install
cp -a %{_payload}/* %{buildroot}/
%post
# PM unconditionally reads /usr/local/<bsp>_bsp/<kver>/kmods.json even with
# package management off; <kver> is known only here, at install time.
for d in /lib/modules/*; do
  [ -d "$d" ] || continue
  k=$(basename "$d")
  mkdir -p "/usr/local/fake_bsp/$k"
  printf '%s' '{"bspKmods": [], "sharedKmods": []}' > "/usr/local/fake_bsp/$k/kmods.json"
done
# Every file the VM's first boot requires. Fail the install (and the image
# build) rather than ship an image that breaks at fboss_init.
for f in /var/facebook/fboss/fruid.json /etc/coop/agent.conf \
    /etc/fboss/fake_platform_manager.json \
    /etc/systemd/system/platform_manager.service.d/fake.conf; do
  [ -s "$f" ] || { echo "fake-ident: missing $f" >&2; exit 1; }
done
[ -n "$(ls /usr/local/fake_bsp/*/kmods.json 2>/dev/null)" ] || { echo "fake-ident: no kmods.json written" >&2; exit 1; }
%files
/var/facebook/fboss/fruid.json
/etc/coop/agent.conf
/etc/fboss/fake_platform_manager.json
/etc/systemd/system/platform_manager.service.d/fake.conf
