#!/bin/bash -x

OUTPUT_DIR="/output"
ARTIFACT="$OUTPUT_DIR/phy_sai-devel.tar"

rm -f "${ARTIFACT}.zst"
echo "SAI_BRCM_PAI_IMPL=1" >phy_sai_build.env
echo "SAI_VERSION=1.15.0" >>phy_sai_build.env
echo "SAI_SDK_VERSION=PHY_SAI_SDK" >>phy_sai_build.env

tar -cvf - phy_sai_build.env | zstd -o "${ARTIFACT}.zst"
rm phy_sai_build.env
