FROM quay.io/centos/centos:stream9

# Build arguments for user creation
ARG USERNAME=root
ARG USER_UID=0
ARG USER_GID=0
ARG USER_SHELL=/bin/bash

# Download and install sccache
RUN version=v0.11.0 && \
    curl -L https://github.com/mozilla/sccache/releases/download/$version/sccache-$version-x86_64-unknown-linux-musl.tar.gz -o sccache.tar.gz && \
    tar xzf sccache.tar.gz && \
    mv sccache-$version-x86_64-unknown-linux-musl/sccache /usr/local/bin/ && \
    rm -rf sccache-$version-x86_64-unknown-linux-musl sccache.tar.gz

# Mount point for extra files.
RUN mkdir -p /var/extras

RUN dnf install -y 'dnf-command(config-manager)'

RUN dnf config-manager --set-enabled crb

RUN dnf install -y epel-release epel-next-release

RUN dnf install -y --allowerasing autoconf automake
RUN dnf install -y --allowerasing binutils binutils-devel
RUN dnf install -y --allowerasing bison flex gdb glibc-devel make
RUN dnf install -y --allowerasing pkgconf pkgconf-m4 pkgconf-pkg-config
RUN dnf install -y --allowerasing redhat-rpm-config rpm-build rpm-sign strace
RUN dnf install -y --allowerasing byacc diffstat intltool ltrace patchutils
RUN dnf install -y --allowerasing pesign valgrind valgrind-devel
RUN dnf install -y --allowerasing bzip2 bzip2-devel
RUN dnf install -y --allowerasing cmake double-conversion double-conversion-devel gperf
RUN dnf install -y --allowerasing libcurl-devel libcurl-minimal
RUN dnf install -y --allowerasing libdwarf libdwarf-devel
RUN dnf install -y --allowerasing libevent-devel
RUN dnf install -y --allowerasing libffi libffi-devel
RUN dnf install -y --allowerasing libnghttp2 libnghttp2-devel
RUN dnf install -y --allowerasing libnl3 libnl3-devel
RUN dnf install -y --allowerasing libsodium-devel libsodium-static
RUN dnf install -y --allowerasing libunwind libunwind-devel
RUN dnf install -y --allowerasing libusb libusb-devel
RUN dnf install -y --allowerasing libzstd libzstd-devel
RUN dnf install -y --allowerasing lz4-devel ncurses-devel ninja-build
RUN dnf install -y --allowerasing openssl openssl-devel openssl-libs
RUN dnf install -y --allowerasing python3 python3-devel
RUN dnf install -y --allowerasing re2 re2-devel
RUN dnf install -y --allowerasing snappy-devel xxhash-devel xz-devel zlib-devel zlib-static
RUN dnf install -y --allowerasing libxml2-devel expat-devel
RUN dnf install -y --allowerasing wget git sudo lsof `basename "$USER_SHELL"`
RUN dnf install -y --allowerasing libcap-devel libmount-devel
RUN dnf install -y --allowerasing gcc-toolset-12 gcc-toolset-12-libasan-devel gcc-toolset-12-libubsan-devel

RUN update-alternatives --install /usr/bin/gcc gcc /opt/rh/gcc-toolset-12/root/usr/bin/gcc 100 \
    --slave /usr/bin/g++ g++ /opt/rh/gcc-toolset-12/root/usr/bin/g++ \
    --slave /usr/bin/c++ c++ /opt/rh/gcc-toolset-12/root/usr/bin/g++ \
    --slave /usr/bin/cc cc /opt/rh/gcc-toolset-12/root/usr/bin/gcc \
    --slave /usr/bin/cpp cpp /opt/rh/gcc-toolset-12/root/usr/bin/cpp \
    --slave /usr/bin/gcov gcov /opt/rh/gcc-toolset-12/root/usr/bin/gcov \
    --slave /usr/bin/gcc-ar gcc-ar /opt/rh/gcc-toolset-12/root/usr/bin/gcc-ar \
    --slave /usr/bin/gcc-nm gcc-nm /opt/rh/gcc-toolset-12/root/usr/bin/gcc-nm \
    --slave /usr/bin/gcc-ranlib gcc-ranlib /opt/rh/gcc-toolset-12/root/usr/bin/gcc-ranlib

RUN dnf download libtool

RUN rpm -ivh --nodeps libtool-*.rpm

RUN rm -f libtool-*.rpm

RUN python3 -m pip install gitpython meson jinja2

# Install clang-format version 21 from LLVM official releases because CentOS Stream 9 is still on version 20.
# Extract the version from .pre-commit-config.yaml to ensure consistency.
COPY .pre-commit-config.yaml /tmp/.pre-commit-config.yaml
RUN LLVM_VERSION=$(grep -A 1 'mirrors-clang-format' /tmp/.pre-commit-config.yaml | grep 'rev:' | sed 's/.*v\([0-9.]*\).*/\1/') && \
    curl -L https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/LLVM-${LLVM_VERSION}-Linux-X64.tar.xz -o /tmp/llvm.tar.xz && \
    tar -xf /tmp/llvm.tar.xz -C /tmp && \
    for i in \
    clang-apply-replacements \
    clang-format \
    clang-format-diff \
    clang-include-cleaner \
    clang-include-fixer \
    clang-refactor \
    clang-reorder-fields \
    clang-tidy \
    clangd \
    find-all-symbols \
    git-clang-format \
    run-clang-tidy; do \
    install -m 0755 /tmp/LLVM-${LLVM_VERSION}-Linux-X64/bin/$i /usr/local/bin/; \
    done && \
    rm -rf /tmp/llvm.tar.xz /tmp/LLVM-${LLVM_VERSION}-Linux-X64 /tmp/.pre-commit-config.yaml

# Use /var/FBOSS as the set location to clone the git repository and store the outputs of the build.
WORKDIR /var/FBOSS/fboss
COPY build/fbcode_builder /var/FBOSS/fboss/build/fbcode_builder
RUN ./build/fbcode_builder/getdeps.py install-system-deps --recursive fboss && \
    rm -rf build

# The following libraries are needed for building SDK
RUN dnf install -y doxygen aspell libyaml-devel vim-common libnsl libnsl2-devel
# Install Perl modules
RUN dnf install -y perl-diagnostics perl-sort perl-English perl-Clone perl-Data-Compare \
    perl-List-MoreUtils perl-Moose perl-YAML perl-namespace-autoclean perl-JSON-XS perl-FindBin \
    perl-Math-BigInt perl-YAML-LibYAML perl-Sys-Hostname perl-Time
RUN ln -s /usr/bin/python3 /usr/bin/python
RUN pip install pyyaml filelock
# ============================================================
COPY requirements-dev.txt /tmp/requirements-dev.txt
RUN pip install -r /tmp/requirements-dev.txt && rm /tmp/requirements-dev.txt

# Configure sudoers to allow wheel group passwordless sudo
RUN echo "%wheel ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

# Create user if USERNAME is not root
RUN if [ "$USERNAME" != "root" ] && [ "$USER_UID" != "0" ]; then \
    groupadd -g $USER_GID $USERNAME && \
    useradd -l -m -u $USER_UID -g $USER_GID -G adm,wheel,users -s $USER_SHELL $USERNAME && \
    chown -R $USERNAME /var/FBOSS; \
    fi

USER ${USERNAME}
