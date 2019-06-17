Name:           fboss_bins
Version:        1
Release:        1%{?dist}
Summary:        FBOSS binaries for CentOS 7 x86_64 4.4LTS

License:        BSD
URL:            https://github.com/facebook/fboss
Source0:        fboss_bins-1.tar.gz

%description

FBOSS binaries for CentOS 7 x86_64 4.4LTS and all library dependencies.

%prep
%setup -q

%build

%install
install -m 0755 -d $RPM_BUILD_ROOT/etc/fboss_bins

# install binaries
install -m 0755 wedge_agent %{buildroot}/etc/fboss_bins/wedge_agent
install -m 0755 bcm_test %{buildroot}/etc/fboss_bins/bcm_test

#install dependent libraries
install -m 0755 libgflags.so.2.2 %{buildroot}/etc/fboss_bins/libgflags.so.2.2
install -m 0755 libglog.so.0 %{buildroot}/etc/fboss_bins/libglog.so.0
install -m 0755 libzstd.so.1.3.8 %{buildroot}/etc/fboss_bins/libzstd.so.1.3.8
install -m 0755 libopennsl.so.1 %{buildroot}/etc/fboss_bins/libopennsl.so.1
install -m 0755 libusb-1.0.so.0 %{buildroot}/etc/fboss_bins/libusb-1.0.so.0
install -m 0755 libnl-3.so.200 %{buildroot}/etc/fboss_bins/libnl-3.so.200
install -m 0755 libcurl.so.4 %{buildroot}/etc/fboss_bins/libcurl.so.4
install -m 0755 libsodium.so.23 %{buildroot}/etc/fboss_bins/libsodium.so.23
install -m 0755 libmnl.so.0 %{buildroot}/etc/fboss_bins/libmnl.so.0

%files
/etc/fboss_bins/wedge_agent
/etc/fboss_bins/bcm_test

/etc/fboss_bins/libgflags.so.2.2
/etc/fboss_bins/libglog.so.0
/etc/fboss_bins/libzstd.so.1.3.8
/etc/fboss_bins/libopennsl.so.1
/etc/fboss_bins/libusb-1.0.so.0
/etc/fboss_bins/libnl-3.so.200
/etc/fboss_bins/libcurl.so.4
/etc/fboss_bins/libsodium.so.23
/etc/fboss_bins/libmnl.so.0

%changelog
* Sat Jun 15 2019 Shrikrishna Khare 1.0.0

Initial rpm release
