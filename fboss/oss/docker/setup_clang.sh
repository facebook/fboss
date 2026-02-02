#!/usr/bin/env bash

# Install clang-format version 21 from LLVM official releases because CentOS Stream 9 is still on version 20.
# Extract the version from .pre-commit-config.yaml to ensure consistency.
# If USE_CLANG=true, install the full LLVM toolchain and configure it as the default compiler.
# Note that we install LLVM under /usr/local/llvm and not directly under /usr/local intentionally,
# because we want to avoid GCC accidently finding LLVM's header, which causes problems with things
# such as libunwind.

LLVM_VERSION=$(grep -A 1 'mirrors-clang-format' /tmp/.pre-commit-config.yaml | grep 'rev:' | sed 's/.*v\([0-9.]*\).*/\1/')

curl -L "https://github.com/llvm/llvm-project/releases/download/llvmorg-${LLVM_VERSION}/LLVM-${LLVM_VERSION}-Linux-X64.tar.xz" -o /tmp/llvm.tar.xz

if [ "$USE_CLANG" = "true" ]; then
    echo "Installing full LLVM toolchain for clang build mode..."

    mkdir -p /usr/local/llvm
    tar -xf /tmp/llvm.tar.xz -C /usr/local/llvm --strip-components=1

    # Create llvm-cov-gcov wrapper script
    echo '#!/bin/sh' > /usr/local/bin/llvm-cov-gcov
    echo 'exec /usr/local/llvm/bin/llvm-cov gcov "$@"' >> /usr/local/bin/llvm-cov-gcov
    chmod +x /usr/local/bin/llvm-cov-gcov

    # Set up alternatives for compiler toolchain
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
        --slave /usr/bin/gcov gcov /usr/local/bin/llvm-cov-gcov

    # Create symlinks for clang tools
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
        run-clang-tidy
    do
        ln -s "/usr/local/llvm/bin/$i" "/usr/local/bin/$i"
    done

    # Create compiler switching scripts
    echo '#!/bin/sh -x' > /usr/local/bin/use-gcc
    echo 'sudo update-alternatives --set gcc /opt/rh/gcc-toolset-12/root/usr/bin/gcc' >> /usr/local/bin/use-gcc

    echo '#!/bin/sh -x' > /usr/local/bin/use-clang
    echo 'sudo update-alternatives --set gcc /usr/local/llvm/bin/clang' >> /usr/local/bin/use-clang

    chmod +x /usr/local/bin/use-gcc /usr/local/bin/use-clang

    # Default to clang
    /usr/local/bin/use-clang
else
    echo "Installing clang-format and related tools only..."

    tar -xf /tmp/llvm.tar.xz -C /tmp

    # Install clang tools
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
        run-clang-tidy
    do
        install -m 0755 "/tmp/LLVM-${LLVM_VERSION}-Linux-X64/bin/$i" /usr/local/bin/
    done

    rm -rf "/tmp/LLVM-${LLVM_VERSION}-Linux-X64"
fi

# Cleanup
rm -rf /tmp/llvm.tar.xz /tmp/.pre-commit-config.yaml
