Name: demo_container
Version: 1.0
Release: 1
Summary: Demo Container
License: GPL
BuildRequires: podman-docker
Requires: podman-docker

%description
Demo building, installing, and running a container on a device inside the Distro Image.

%prep
cp %{_sourcedir}/Dockerfile .
cp %{_sourcedir}/time.py .

%build
docker build . -t demo_container
rm -f demo_container.tar
docker save demo_container -o demo_container.tar

%install
%{__install} -D -m 644 demo_container.tar %{buildroot}/usr/share/demo_container/demo_container.tar
%{__install} -D -m 644 %{_sourcedir}/demo_container.service %{buildroot}/usr/lib/systemd/system/demo_container.service
mkdir -p %{buildroot}/var/clock

%files
%defattr(-,root,root,-)
/var/clock
%config(missingok) /usr/share/demo_container/demo_container.tar
/usr/lib/systemd/system/demo_container.service

%post
docker load -i /usr/share/demo_container/demo_container.tar
rm -rf /usr/share/demo_container
systemctl enable demo_container.service
