# Navigate to the right directory
cd /var/FBOSS/fboss

# Prepare BRCM_PAI SDK artifacts directory for linking
mkdir -p /var/FBOSS/pai_impl
mkdir -p /var/FBOSS/pai_impl/lib
mkdir -p /var/FBOSS/pai_impl/include

# Copy the three sdk artifacts to `lib` directory
# You should see the following three precompiled artifacts as follow
# ls /var/FBOSS/pai_impl/lib
# libepdm.a  libpai.a  libphymodepil.a

# Copy the header folders from PAI sdk to `include directory`
# NOTE: Adjust the correct directory based on your setup
cp -r /opt/sdk/PAI_3.15/inc/sai /var/FBOSS/pai_impl/include
cp -r /opt/sdk/PAI_3.15/inc/pai_macsec /var/FBOSS/pai_impl/include

# Set environment variables to build the qsfp targets with the BRCM PAI
# Make sure to unset any existing variables to link with ASIC SDK as PHY
# SDK needs different sai
unset SAI_BRCM_IMPL
unset CHENAB_SAI_SDK
unset SAI_TAJO_IMPL
export SAI_BRCM_PAI_IMPL=1

# Start the build with specific cmake-target `qsfp_targets`
time ./build/fbcode_builder/getdeps.py build --allow-system-packages \
--extra-cmake-defines='{"CMAKE_BUILD_TYPE": "MinSizeRel", "CMAKE_CXX_STANDARD": "20"}' \
--scratch-path /var/FBOSS/tmp_bld_dir --cmake-target qsfp_targets fboss
