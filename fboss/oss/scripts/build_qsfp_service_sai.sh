#!/bin/bash -x

OUTPUT_DIR="/output"
ARTIFACT="$OUTPUT_DIR/qsfp_service_sai-devel.tar"

rm -f "${ARTIFACT}.zst"
echo "SAI_BRCM_PAI_IMPL=1" >qsfp_service_sai_build.env
echo "SAI_VERSION=1.15.0" >>qsfp_service_sai_build.env
echo "SAI_SDK_VERSION=PHY_SAI_SDK" >>qsfp_service_sai_build.env

tar -cvf - qsfp_service_sai_build.env | zstd -o "${ARTIFACT}.zst"
rm qsfp_service_sai_build.env
