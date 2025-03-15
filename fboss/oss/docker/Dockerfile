FROM quay.io/centos/centos:stream9

RUN dnf install git sudo lsof -y

# Use /var/FBOSS as the set location to clone the git repository and store the outputs of the build.
WORKDIR /var/FBOSS/

RUN mkdir -p /var/FBOSS/fboss
COPY . fboss
WORKDIR fboss

RUN rm -rf build/deps/github_hashes/
RUN tar xvzf fboss/oss/stable_commits/latest_stable_hashes.tar.gz

RUN dnf install -y 'dnf-command(config-manager)'
RUN dnf config-manager --set-enabled crb
RUN dnf install -y epel-release epel-next-release
RUN sudo dnf install -y autoconf automake binutils binutils-devel bzip2 \
    bzip2-devel cmake double-conversion double-conversion-devel libcurl-devel \
    libcurl-minimal libdwarf libdwarf-devel libevent-devel libffi libffi-devel \
    libnghttp2 libnghttp2-devel libnl3 libnl3-devel libsodium-devel \
    libsodium-static libtool libunwind libunwind-devel libusb libusb-devel \ 
    libzstd libzstd-devel lz4-devel ncurses-devel ninja-build openssl \
    openssl-devel openssl-libs python3 python3-devel re2 re2-devel \
    snappy-devel xxhash-devel xz-devel zlib-devel zlib-static -y --allowerasing
RUN ./build/fbcode_builder/getdeps.py install-system-deps --recursive fboss
RUN dnf install bison flex -y
RUN dnf group install "Development Tools" -y
RUN dnf install openssl* -y
RUN python3 -m pip install gitpython
RUN python3 -m pip install meson
RUN python3 -m pip install jinja2
RUN dnf install gperf libcap-devel libmount-devel -y

