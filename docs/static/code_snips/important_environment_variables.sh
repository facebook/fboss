# The following environment variables should be set depending on which platform
# and SDK version you are building

# Set to 1 if building against brcm-sai SDK
export SAI_BRCM_IMPL=1

# Can be omitted if you are using SAI 1.16.3. If using a more recent version of
# SAI from https://github.com/opencomputeproject/SAI, this should be set to the
# semantic version e.g. 1.16.1
export SAI_VERSION="1.16.1"

# Should be set to a string depending on which version of the brcm-sai SDK you
# are building. Default value is "SAI_VERSION_11_0_EA_DNX_ODP".
# Supported values can be found in
# https://github.com/facebook/fboss/blob/main/fboss/agent/hw/sai/api/SaiVersion
# but are listed below for convenience:

# SAI_VERSION_8_2_0_0_ODP
# SAI_VERSION_8_2_0_0_SIM_ODP
# SAI_VERSION_9_2_0_0_ODP
# SAI_VERSION_9_0_EA_SIM_ODP
# SAI_VERSION_10_0_EA_ODP
# SAI_VERSION_10_0_EA_SIM_ODP
# SAI_VERSION_10_2_0_0_ODP
# SAI_VERSION_11_0_EA_ODP
# SAI_VERSION_11_0_EA_SIM_ODP
# SAI_VERSION_11_3_0_0_ODP
# SAI_VERSION_11_7_0_0_ODP
# SAI_VERSION_10_0_EA_DNX_ODP
# SAI_VERSION_10_0_EA_DNX_SIM_ODP
# SAI_VERSION_11_0_EA_DNX_ODP
# SAI_VERSION_11_0_EA_DNX_SIM_ODP
# SAI_VERSION_11_3_0_0_DNX_ODP
# SAI_VERSION_11_7_0_0_DNX_ODP
# SAI_VERSION_12_0_EA_DNX_ODP
# SAI_VERSION_13_0_EA_ODP
# SAI_VERSION_14_0_EA_ODP

# Set the SAI SDK version
export SAI_SDK_VERSION="SAI_VERSION_14_0_EA_ODP"

# Set to 1 to install benchmark binaries
export BENCHMARK_INSTALL=1

# Set to 1 to skip installing FBOSS artifacts
export SKIP_ALL_INSTALL=1

# Set to 1 to enable sanitization checking
export WITH_ASAN=1
