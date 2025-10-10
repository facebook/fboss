#
# Copyright 2004-present Facebook. All Rights Reserved.
#
#
# Constants defines for sdkcastle
#


# List of ASICs for BRCM DNX platforms
BRCM_DNX_ASICS = ["jericho3", "ramon3"]

# Team mappings for different test types in meta-internal mode
TEST_TYPE_TEAM_MAPPING = {
    "hw": "sai_test",
    "agent": "sai_agent_test",
    "link": "fboss_link",
    "benchmark": "sai_bench",
    "n-warmboot": "sai_agent_test",
    "config": "sai_agent_test",
}
