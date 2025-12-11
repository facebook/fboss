FROM quay.io/centos/centos:stream9

# Build arguments for user creation
ARG USERNAME=root
ARG USER_UID=0
ARG USER_GID=0
ARG USER_SHELL=/bin/bash
ARG USE_CLANG=false

# Mount point for extra files.
RUN mkdir -p /var/extras

RUN dnf install -y 'dnf-command(config-manager)' && \
    dnf config-manager --set-enabled crb && \
    dnf install -y epel-release epel-next-release && \
    dnf install -y --allowerasing \
    autoconf automake bison flex gdb glibc-devel make \
    pkgconf pkgconf-m4 pkgconf-pkg-config redhat-rpm-config rpm-build rpm-sign strace \
    byacc diffstat intltool ltrace patchutils \
    pesign valgrind valgrind-devel \
    bzip2 bzip2-devel cmake double-conversion double-conversion-devel gperf \
    libcurl-devel libcurl-minimal libdwarf libdwarf-devel libevent-devel \
    libffi libffi-devel libnghttp2 libnghttp2-devel libnl3 libnl3-devel \
    libsodium-devel libsodium-static libunwind libunwind-devel libusb libusb-devel \
    libzstd libzstd-devel lz4-devel ncurses-devel ninja-build \
    openssl openssl-devel openssl-libs python3 python3-devel \
    re2 re2-devel snappy-devel xxhash-devel xz-devel zlib-devel zlib-static \
    libxml2-devel expat-devel wget git sudo lsof `basename "$USER_SHELL"` \
    libcap-devel libmount-devel \
    gcc-toolset-12 gcc-toolset-12-libasan-devel gcc-toolset-12-libubsan-devel && \
    rpm -e binutils --nodeps && \
    update-alternatives --install /usr/bin/gcc gcc /opt/rh/gcc-toolset-12/root/usr/bin/gcc 100 \
    --slave /usr/bin/g++ g++ /opt/rh/gcc-toolset-12/root/usr/bin/g++ \
    --slave /usr/bin/c++ c++ /opt/rh/gcc-toolset-12/root/usr/bin/g++ \
    --slave /usr/bin/cc cc /opt/rh/gcc-toolset-12/root/usr/bin/gcc \
    --slave /usr/bin/cpp cpp /opt/rh/gcc-toolset-12/root/usr/bin/cpp \
    --slave /usr/bin/ld ld /opt/rh/gcc-toolset-12/root/usr/bin/ld \
    --slave /usr/bin/ar ar /opt/rh/gcc-toolset-12/root/usr/bin/ar \
    --slave /usr/bin/nm nm /opt/rh/gcc-toolset-12/root/usr/bin/nm \
    --slave /usr/bin/ranlib ranlib /opt/rh/gcc-toolset-12/root/usr/bin/ranlib \
    --slave /usr/bin/objcopy objcopy /opt/rh/gcc-toolset-12/root/usr/bin/objcopy \
    --slave /usr/bin/objdump objdump /opt/rh/gcc-toolset-12/root/usr/bin/objdump \
    --slave /usr/bin/readelf readelf /opt/rh/gcc-toolset-12/root/usr/bin/readelf \
    --slave /usr/bin/strip strip /opt/rh/gcc-toolset-12/root/usr/bin/strip \
    --slave /usr/bin/gcov gcov /opt/rh/gcc-toolset-12/root/usr/bin/gcov \
    --slave /usr/bin/gcc-ar gcc-ar /opt/rh/gcc-toolset-12/root/usr/bin/gcc-ar \
    --slave /usr/bin/gcc-nm gcc-nm /opt/rh/gcc-toolset-12/root/usr/bin/gcc-nm \
    --slave /usr/bin/gcc-ranlib gcc-ranlib /opt/rh/gcc-toolset-12/root/usr/bin/gcc-ranlib && \
    dnf download libtool && \
    rpm -ivh --nodeps libtool-*.rpm && \
    rm -f libtool-*.rpm && \
    python3 -m pip install boto3 botocore gitpython meson jinja2

# Install clang-format version 21 from LLVM official releases because CentOS Stream 9 is still on version 20.
# Extract the version from .pre-commit-config.yaml to ensure consistency.
# If USE_CLANG=true, install the full LLVM toolchain and configure it as the default compiler.
# Note that we install LLVM under /usr/local/llvm and not directly under /usr/local intentionally,
# because we want to avoid GCC accidently finding LLVM's header, which causes problems with things
# such as libunwind.
COPY .pre-commit-config.yaml /tmp/.pre-commit-config.yaml
RUN LLVM_VERSION=$(grep -A 1 'mirrors-clang-format' /tmp/.pre-commit-config.yaml | grep 'rev:' | sed 's/.*v\([0-9.]*\).*/\1/') && \
    curl -L https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/LLVM-${LLVM_VERSION}-Linux-X64.tar.xz -o /tmp/llvm.tar.xz && \
    if [ "$USE_CLANG" = "true" ]; then \
    echo "Installing full LLVM toolchain for clang build mode..." && \
    mkdir -p /usr/local/llvm && \
    tar -xf /tmp/llvm.tar.xz -C /usr/local/llvm --strip-components=1 && \
    echo '#!/bin/sh' > /usr/local/bin/llvm-cov-gcov && \
    echo 'exec /usr/local/llvm/bin/llvm-cov gcov "$@"' >> /usr/local/bin/llvm-cov-gcov && \
    chmod +x /usr/local/bin/llvm-cov-gcov && \
    update-alternatives --install /usr/bin/gcc gcc /usr/local/llvm/bin/clang 200 \
    --slave /usr/bin/g++ g++ /usr/local/llvm/bin/clang++ \
    --slave /usr/bin/c++ c++ /usr/local/llvm/bin/clang++ \
    --slave /usr/bin/cc cc /usr/local/llvm/bin/clang \
    --slave /usr/bin/cpp cpp /usr/local/llvm/bin/clang-cpp \
    --slave /usr/bin/ld ld /usr/local/llvm/bin/lld \
    --slave /usr/bin/ar ar /usr/local/llvm/bin/llvm-ar \
    --slave /usr/bin/nm nm /usr/local/llvm/bin/llvm-nm \
    --slave /usr/bin/ranlib ranlib /usr/local/llvm/bin/llvm-ranlib \
    --slave /usr/bin/objcopy objcopy /usr/local/llvm/bin/llvm-objcopy \
    --slave /usr/bin/objdump objdump /usr/local/llvm/bin/llvm-objdump \
    --slave /usr/bin/readelf readelf /usr/local/llvm/bin/llvm-readelf \
    --slave /usr/bin/strip strip /usr/local/llvm/bin/llvm-strip \
    --slave /usr/bin/gcov gcov /usr/local/bin/llvm-cov-gcov && \
    for i in \
    clang-apply-replacements \
    clang-format \
    clang-include-cleaner \
    clang-include-fixer \
    clang-refactor \
    clang-reorder-fields \
    clang-tidy \
    clangd \
    find-all-symbols \
    git-clang-format \
    run-clang-tidy; do \
    ln -s /usr/local/llvm/bin/$i /usr/local/bin/$i; \
    done && \
    echo '#!/bin/sh -x' > /usr/local/bin/use-gcc && \
    echo 'sudo update-alternatives --set gcc /opt/rh/gcc-toolset-12/root/usr/bin/gcc' >> /usr/local/bin/use-gcc && \
    echo '#!/bin/sh -x' > /usr/local/bin/use-clang && \
    echo 'sudo update-alternatives --set gcc /usr/local/llvm/bin/clang' >> /usr/local/bin/use-clang && \
    chmod +x /usr/local/bin/use-gcc /usr/local/bin/use-clang && \
    /usr/local/bin/use-clang; \
    else \
    echo "Installing clang-format and related tools only..." && \
    tar -xf /tmp/llvm.tar.xz -C /tmp && \
    for i in \
    clang-apply-replacements \
    clang-format \
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
    rm -rf /tmp/LLVM-${LLVM_VERSION}-Linux-X64; \
    fi && \
    rm -rf /tmp/llvm.tar.xz /tmp/.pre-commit-config.yaml

# Use /var/FBOSS as the set location to clone the git repository and store the outputs of the build.
WORKDIR /var/FBOSS/fboss
COPY build/fbcode_builder /var/FBOSS/fboss/build/fbcode_builder
RUN if [ "$USE_CLANG" = "true" ]; then \
    sed 's/^binutils/#binutils/' -i build/fbcode_builder/manifests/*; \
    fi && \
    ./build/fbcode_builder/getdeps.py install-system-deps --recursive fboss && \
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
