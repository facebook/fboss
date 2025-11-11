FROM quay.io/centos/centos:stream9

RUN dnf install git sudo lsof -y

# Use /var/FBOSS as the set location to clone the git repository and store the outputs of the build.
WORKDIR /var/FBOSS/

# Download and install sccache
RUN version=v0.11.0 && \
    curl -L https://github.com/mozilla/sccache/releases/download/$version/sccache-$version-x86_64-unknown-linux-musl.tar.gz -o sccache.tar.gz && \
    tar xzf sccache.tar.gz && \
    mv sccache-$version-x86_64-unknown-linux-musl/sccache /usr/local/bin/ && \
    rm -rf sccache-$version-x86_64-unknown-linux-musl sccache.tar.gz

# Mount point for extra files.
RUN mkdir -p /var/extras

RUN mkdir -p /var/FBOSS/fboss
COPY . fboss
WORKDIR fboss

RUN rm -rf build/deps/github_hashes/
RUN tar --no-same-owner -xvzf fboss/oss/stable_commits/latest_stable_hashes.tar.gz

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
RUN python3 -m pip install gitpython
RUN python3 -m pip install meson
RUN python3 -m pip install jinja2
RUN dnf install gperf libcap-devel libmount-devel -y
RUN dnf install gcc-toolset-12 gcc-toolset-12-libasan-devel gcc-toolset-12-libubsan-devel -y
RUN echo "source /opt/rh/gcc-toolset-12/enable" >> ~/.bashrc

RUN rm -rf /var/FBOSS/fboss/*

# The following libraries are needed for building SDK
RUN dnf install -y doxygen aspell libyaml-devel vim-common libnsl libnsl2-devel
# Install Perl modules
RUN dnf install -y perl-diagnostics perl-sort perl-English perl-Clone perl-Data-Compare \
    perl-List-MoreUtils perl-Moose perl-YAML perl-namespace-autoclean perl-JSON-XS perl-FindBin \
    perl-Math-BigInt perl-YAML-LibYAML perl-Sys-Hostname perl-Time
RUN dnf install -y libxml2-devel expat-devel wget
RUN ln -s /usr/bin/python3 /usr/bin/python
RUN pip install pyyaml filelock
# ============================================================
