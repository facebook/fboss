#!/usr/bin/env python3
# @noautodeps
# (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

# Common argparse option strings shared across run_test.py and multiple runners
OPT_ARG_COLDBOOT = "--coldboot_only"
OPT_ARG_FILTER = "--filter"
OPT_ARG_FILTER_FILE = "--filter_file"
OPT_ARG_PROFILE = "--profile"
OPT_ARG_LIST_TESTS = "--list_tests"
OPT_ARG_CONFIG_FILE = "--config"
OPT_ARG_QSFP_CONFIG_FILE = "--qsfp-config"
OPT_ARG_PLATFORM_MAPPING_OVERRIDE_PATH = "--platform_mapping_override_path"
OPT_ARG_BSP_PLATFORM_MAPPING_OVERRIDE_PATH = "--bsp_platform_mapping_override_path"
OPT_ARG_SAI_REPLAYER_LOGGING = "--sai_replayer_logging"
OPT_ARG_SKIP_KNOWN_BAD_TESTS = "--skip-known-bad-tests"
OPT_ARG_MGT_IF = "--mgmt-if"
OPT_ARG_FRUID_PATH = "--fruid-path"
OPT_ARG_SIMULATOR = "--simulator"
OPT_ARG_SAI_LOGGING = "--sai_logging"
OPT_ARG_FBOSS_LOGGING = "--fboss_logging"
OPT_KNOWN_BAD_TESTS_FILE = "--known-bad-tests-file"
OPT_UNSUPPORTED_TESTS_FILE = "--unsupported-tests-file"
OPT_ARG_SETUP_CB = "--setup-for-coldboot"
OPT_ARG_SETUP_WB = "--setup-for-warmboot"
OPT_ARG_TEST_RUN_TIMEOUT = "--test-run-timeout"
OPT_ARG_DISABLE_FSDB = "--disable-fsdb"
OPT_ARG_FSDB_CONFIG_FILE = "--fsdb-config"

# Subcommand names
SUB_CMD_BCM = "bcm"
SUB_CMD_SAI = "sai"
SUB_CMD_QSFP = "qsfp"
SUB_CMD_LINK = "link"
SUB_CMD_SAI_AGENT = "sai_agent"
SUB_CMD_SAI_AGENT_SCALE = "sai_agent_scale"
SUB_CMD_SAI_INVARIANT_AGENT = "sai_invariant_agent"
SUB_CMD_PLATFORM = "platform"
SUB_CMD_FBOSS2_INTEGRATION = "fboss2_integration"
SUB_CMD_BENCHMARK = "benchmark"

# Subcommand args shared across multiple runners
SUB_ARG_AGENT_RUN_MODE = "--agent-run-mode"
SUB_ARG_AGENT_RUN_MODE_MONO = "mono"
SUB_ARG_AGENT_RUN_MODE_MULTI = "multi_switch"
SUB_ARG_NUM_NPUS = "--num-npus"

XGS_SIMULATOR_ASICS = ["th3", "th4", "th4_b0", "th5"]
DNX_SIMULATOR_ASICS = ["j3"]
ALL_SIMUALTOR_ASICS_STR = "|".join(XGS_SIMULATOR_ASICS + DNX_SIMULATOR_ASICS)

DEFAULT_TEST_RUN_TIMEOUT_IN_SECOND = 1200
