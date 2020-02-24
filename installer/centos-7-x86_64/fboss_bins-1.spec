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
install -m 0755 -d $RPM_BUILD_ROOT/opt/fboss

# install binaries
install -m 0755 wedge_agent %{buildroot}/opt/fboss/wedge_agent
install -m 0755 bcm_test %{buildroot}/opt/fboss/bcm_test
install -m 0755 sai_test-fake-1.5.0 %{buildroot}/opt/fboss/sai_test-fake-1.5.0

#install dependent libraries
install -m 0755 libgflags.so.2.2 %{buildroot}/opt/fboss/libgflags.so.2.2
install -m 0755 libglog.so.0 %{buildroot}/opt/fboss/libglog.so.0
install -m 0755 libzstd.so.1.3.8 %{buildroot}/opt/fboss/libzstd.so.1.3.8
install -m 0755 libusb-1.0.so.0 %{buildroot}/opt/fboss/libusb-1.0.so.0
install -m 0755 libnl-3.so.200 %{buildroot}/opt/fboss/libnl-3.so.200
install -m 0755 libcurl.so.4 %{buildroot}/opt/fboss/libcurl.so.4
install -m 0755 libsodium.so.23 %{buildroot}/opt/fboss/libsodium.so.23
install -m 0755 libmnl.so.0 %{buildroot}/opt/fboss/libmnl.so.0
install -m 0755 libnghttp2.so.14 %{buildroot}/opt/fboss/libnghttp2.so.14

%files
/opt/fboss/wedge_agent
/opt/fboss/bcm_test
/opt/fboss/sai_test-fake-1.5.0

/opt/fboss/libgflags.so.2.2
/opt/fboss/libglog.so.0
/opt/fboss/libzstd.so.1.3.8
/opt/fboss/libusb-1.0.so.0
/opt/fboss/libnl-3.so.200
/opt/fboss/libcurl.so.4
/opt/fboss/libsodium.so.23
/opt/fboss/libmnl.so.0
/opt/fboss/libnghttp2.so.14

%changelog
* Sat Feb 22  2020 Shrikrishna Khare 1.0.3

Initial rpm release
