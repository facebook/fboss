#!/bin/bash
set -e

# We need to build a custom iPXE executable for IPv6 so we can include the autoexecipv6.ipxe script which works around
# the lack of a 'source server' during IPv6 boot.
if [ ! -d ipxe ]; then
  git clone https://github.com/ipxe/ipxe.git
fi

cd ipxe/src
make bin-x86_64-efi/ipxe.efi
cp bin-x86_64-efi/ipxe.efi ../../ipxev4.efi
make bin-x86_64-efi/ipxe.efi EMBED=../../autoipv6.ipxe
cp bin-x86_64-efi/ipxe.efi ../../ipxev6.efi
