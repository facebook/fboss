# Hand-written Bazel targets for SAI hw_test thrift handler library.
# Loaded by the auto-generated BUILD.bazel via the
# "# bazelify: include sai_hw_test_impl.bzl:sai_hw_test_impl" annotation in BUCK.
#
# This defines agent_hw_test_thrift_handler, the SAI implementation of
# utility::createHwTestThriftHandler() called by HwAgentMain.cpp.
# Corresponds to agent_hw_test_thrift_handler in cmake/AgentHwSaiHwTest.cmake.

load("@rules_cc//cc:defs.bzl", "cc_library")

def sai_hw_test_impl():
    # SAI implementation of agent_hw_test_thrift_handler.
    # Required by fboss_hw_agent-sai_impl: HwAgentMain.cpp unconditionally
    # references utility::createHwTestThriftHandler (guarded only at runtime).
    cc_library(
        name = "agent_hw_test_thrift_handler",
        srcs = [
            # ThriftHandler entry point and per-feature handlers
            "HwTestThriftHandler.cpp",
            "HwTestAclUtilsThriftHandler.cpp",
            "HwTestAqmUtilsThriftHandler.cpp",
            "HwTestArsFlowletThriftHandler.cpp",
            "HwTestCommonUtilsThriftHandler.cpp",
            "HwTestEcmpUtilsThriftHandler.cpp",
            "HwTestFlowletUtilsThriftHandler.cpp",
            "HwTestMirrorUtilsThriftHandler.cpp",
            "HwTestNeighborUtilsThriftHandler.cpp",
            "HwTestPortUtilsThriftHandler.cpp",
            "HwTestPtpTcUtilsThriftHandler.cpp",
            "HwTestRouteUtilsThriftHandler.cpp",
            "HwTestSwitchingModeUtilsThriftHandler.cpp",
            "HwTestTamUtilsThriftHandler.cpp",
            "HwTestTeFlowUtilsThriftHandler.cpp",
            "HwTestUdfUtilsThriftHandler.cpp",
            "HwTestVoqSwitchUtilsThriftHandler.cpp",
            # SAI utility implementations called by the ThriftHandler files.
            # Only include utils that DON'T duplicate symbols already defined
            # in the corresponding ThriftHandler file.  Excluded (duplicates):
            # HwTestAclUtils, HwTestFlowletSwitchingUtils, HwTestMirrorUtils,
            # HwTestRouteUtils, HwTestTamUtils.
            "HwTestAqmUtils.cpp",
            "HwTestEcmpUtils.cpp",
            "HwTestFabricUtils.cpp",
            "HwTestNeighborUtils.cpp",
            "HwTestPortUtils.cpp",
            "HwTestPtpTcUtils.cpp",
            "HwTestSwitchingModeUtils.cpp",
            "HwTestTeFlowUtils.cpp",
            "HwTestUdfUtils.cpp",
        ],
        deps = [
            "//fboss/agent:hw_switch",
            "//fboss/agent/hw/sai/diag:diag_lib",
            "//fboss/agent/hw/sai/switch:sai_switch",
            "//fboss/agent/hw/test:hw_test_thrift_handler_h",
            "//fboss/agent/if:agent_hw_test_ctrl",
            "//fboss/agent/platforms/common/utils:wedge-led-utils",
            "//fboss/agent/test/utils:acl_test_utils",
        ],
        visibility = ["//visibility:public"],
    )
