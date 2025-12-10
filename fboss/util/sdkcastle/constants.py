#
# Copyright 2004-present Facebook. All Rights Reserved.
#
#
# Constants defines for sdkcastle
#

# pyre-unsafe

# J3AI Rev non-A0 ASIC string
J3AI_REV_NOT_A0 = "asic_rev_not_A0=yes"

# List of ASICs for BRCM DNX platforms
BRCM_DNX_ASICS = ["jericho3", "ramon3"]

# Team mappings for different test types in meta-internal mode
TEST_TYPE_TEAM_MAPPING = {
    "hw": "sai_test",
    "agent": "sai_agent_test",
    "agent-scale": "sai_agent_scale_test",
    "link": "fboss_link",
    "benchmark": "sai_bench",
    "n-warmboot": "sai_agent_test",
    "config": "sai_agent_test",
}

# Mapping between ASIC and its benchmark config (if config is different from ASIC name)
BENCHMARK_ASIC_CONFIG = {
    "tomahawk": "tomahawk_pentium",
    "tomahawk3": "tomahawk3_pciegen3",
}
