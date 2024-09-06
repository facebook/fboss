load("@fbcode_macros//build_defs:auto_headers.bzl", "AutoHeaders")
load("@fbcode_macros//build_defs:cpp_binary.bzl", "cpp_binary")
load("@fbcode_macros//build_defs:cpp_library.bzl", "cpp_library")
load("//fboss/agent/hw/sai/impl:impl.bzl", "get_all_impls_for", "get_all_npu_impls", "get_link_group_map", "impl_category_suffix", "to_impl_lib_name", "to_impl_suffix", "to_versions")
load("//fboss/agent/hw/sai/switch:switch.bzl", "sai_switch_dependent_name", "sai_switch_lib_name")

def _sai_switch_ensemble(sai_impl, is_npu):
    switch_lib_name = sai_switch_lib_name(sai_impl, is_npu)
    thrift_test_handler = sai_switch_dependent_name("thrift_test_handler", sai_impl, is_npu)
    return cpp_library(
        name = "{}".format(get_switch_ensemble_name(sai_impl, is_npu)),
        srcs = [
            "HwSwitchEnsembleFactory.cpp",
            "SaiSwitchEnsemble.cpp",
        ],
        # Link whole (--whole_archive), since ensemble may get used
        # in contexts where we are just combining libs - e.g. a h/w
        # switch agnostic benchmark lib and a sai_switch_ensemble to
        # plugin the right ensemble. Without whole_archive, depending
        # on the linking order symbols from sai_switch_ensemble which
        # are needed by the hw switch agnostic benchmark lib maybe
        # dropped.
        link_whole = True,
        exported_deps = [
            "//fboss/agent:core",
            "//fboss/agent:setup_thrift",
            "//fboss/agent/hw/test:hw_switch_ensemble_factory",
            "//fboss/agent/test:linkstate_toggler",
            "//fboss/agent/hw/sai/switch:{}".format(
                switch_lib_name,
            ),
            "//fboss/agent/hw/sai/hw_test:{}".format(
                thrift_test_handler,
            ),
            "//fboss/agent/hw/test:config_factory",
            "//fboss/agent/platforms/sai:{}".format(
                sai_switch_dependent_name("sai_platform", sai_impl, is_npu),
            ),
            "//folly/io/async:async_signal_handler",
        ],
        versions = to_versions(sai_impl),
    )

def _sai_phy_capabilities(sai_impl, is_npu):
    switch_lib_name = sai_switch_lib_name(sai_impl, is_npu)
    return cpp_library(
        name = "{}".format(get_switch_phy_capabilities_name(sai_impl, is_npu)),
        srcs = [
            "PhyCapabilities.cpp",
        ],
        exported_deps = [
            "//fboss/agent/hw/test:phy_capabilities",
            "//fboss/agent/hw/sai/switch:{}".format(
                switch_lib_name,
            ),
        ],
        versions = to_versions(sai_impl),
    )

def get_switch_ensemble_name(sai_impl, is_npu):
    return sai_switch_dependent_name("sai_switch_ensemble", sai_impl, is_npu)

def _switch_ensembles(is_npu):
    all_impls = get_all_impls_for(is_npu)
    for sai_impl in all_impls:
        _sai_switch_ensemble(sai_impl, is_npu)

def get_switch_phy_capabilities_name(sai_impl, is_npu):
    return sai_switch_dependent_name("sai_switch_phy_capabilities", sai_impl, is_npu)

def _switch_phy_capabilities(is_npu):
    all_impls = get_all_impls_for(is_npu)
    for sai_impl in all_impls:
        _sai_phy_capabilities(sai_impl, is_npu)

def all_switch_ensembles():
    _switch_ensembles(is_npu = True)
    _switch_ensembles(is_npu = False)

def all_phy_capabilities():
    _switch_phy_capabilities(is_npu = True)
    _switch_phy_capabilities(is_npu = False)

def all_acl_utils():
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        impl_suffix = to_impl_suffix(sai_impl)
        switch_lib_name = sai_switch_lib_name(sai_impl, True)
        cpp_library(
            name = "sai_acl_utils{}".format(impl_suffix),
            srcs = [
                "HwTestAclUtils.cpp",
            ],
            exported_deps = [
                "//fboss/agent/test/utils:queue_per_host_test_utils",
                "//fboss/agent/hw/test:hw_test_acl_utils",
                "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
            ],
            versions = to_versions(sai_impl),
        )

def all_port_utils():
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        impl_suffix = to_impl_suffix(sai_impl)
        switch_lib_name = sai_switch_lib_name(sai_impl, True)
        cpp_library(
            name = "sai_port_utils{}".format(impl_suffix),
            srcs = [
                "HwTestPortUtils.cpp",
            ],
            exported_deps = [
                "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
                "//fboss/agent/hw/test:hw_test_port_utils",
                "//fboss/agent/platforms/sai:{}".format(
                    sai_switch_dependent_name("sai_platform", sai_impl, True),
                ),
            ],
            versions = to_versions(sai_impl),
        )

def all_ecmp_utils():
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        impl_suffix = to_impl_suffix(sai_impl)
        switch_lib_name = sai_switch_lib_name(sai_impl, True)
        cpp_library(
            name = "sai_ecmp_utils{}".format(impl_suffix),
            srcs = [
                "HwTestEcmpUtils.cpp",
            ],
            exported_deps = [
                "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
                "//fboss/agent/hw/test:hw_test_ecmp_utils",
            ],
            versions = to_versions(sai_impl),
        )

def all_fabric_utils():
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        impl_suffix = to_impl_suffix(sai_impl)
        switch_lib_name = sai_switch_lib_name(sai_impl, True)
        cpp_library(
            name = "sai_fabric_utils{}".format(impl_suffix),
            srcs = [
                "HwTestFabricUtils.cpp",
            ],
            exported_deps = [
                "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
                "//fboss/agent/hw/test:hw_test_fabric_utils",
            ],
            versions = to_versions(sai_impl),
        )

def all_teflow_utils():
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        impl_suffix = to_impl_suffix(sai_impl)
        switch_lib_name = sai_switch_lib_name(sai_impl, True)
        cpp_library(
            name = "sai_teflow_utils{}".format(impl_suffix),
            srcs = [
                "HwTestTeFlowUtils.cpp",
            ],
            exported_deps = [
                "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
                "//fboss/agent/hw/test:hw_test_te_flow_utils",
            ],
            versions = to_versions(sai_impl),
        )

def all_trunk_utils():
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        impl_suffix = to_impl_suffix(sai_impl)
        switch_lib_name = sai_switch_lib_name(sai_impl, True)
        cpp_library(
            name = "sai_trunk_utils{}".format(impl_suffix),
            srcs = [
                "HwTestTrunkUtils.cpp",
            ],
            headers = [
                "SaiSwitchEnsemble.h",
            ],
            exported_deps = [
                "//fboss/agent/hw/sai/diag:diag_lib",
                "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
                "//fboss/agent/hw/sai/switch:if-cpp2-types",
                "//fboss/agent/hw/test:hw_test_trunk_utils",
                "//fboss/agent/hw/test:config_factory",
                "//fboss/agent/hw/sai/switch:if-cpp2-services",
            ],
            versions = to_versions(sai_impl),
        )

def all_ptp_tc_utils():
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        impl_suffix = to_impl_suffix(sai_impl)
        switch_lib_name = sai_switch_lib_name(sai_impl, True)
        cpp_library(
            name = "sai_ptp_tc_utils{}".format(impl_suffix),
            srcs = [
                "HwTestPtpTcUtils.cpp",
            ],
            exported_deps = [
                "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
                "//fboss/agent/hw/test:hw_test_ptp_tc_utils",
            ],
            versions = to_versions(sai_impl),
        )

def all_udf_utils():
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        impl_suffix = to_impl_suffix(sai_impl)
        switch_lib_name = sai_switch_lib_name(sai_impl, True)
        cpp_library(
            name = "sai_udf_utils{}".format(impl_suffix),
            srcs = [
                "HwTestUdfUtils.cpp",
            ],
            exported_deps = [
                "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
                "//fboss/agent/hw/test:hw_test_udf_utils",
            ],
            versions = to_versions(sai_impl),
        )

def all_packet_trap_helper():
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        impl_suffix = to_impl_suffix(sai_impl)
        switch_lib_name = sai_switch_lib_name(sai_impl, True)
        cpp_library(
            name = "sai_packet_trap_helper{}".format(impl_suffix),
            srcs = [
                "HwTestPacketTrapEntry.cpp",
            ],
            exported_deps = [
                "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
                "//fboss/agent/hw/test:hw_test_packet_trap_entry",
            ],
            versions = to_versions(sai_impl),
        )

def all_copp_utils():
    all_impls = get_all_npu_impls()
    for sai_impl in all_impls:
        impl_suffix = to_impl_suffix(sai_impl)
        switch_lib_name = sai_switch_lib_name(sai_impl, True)
        cpp_library(
            name = "sai_copp_utils{}".format(impl_suffix),
            srcs = [
                "HwTestCoppUtils.cpp",
            ],
            link_whole = True,  # T76171234
            exported_deps = [
                "//fboss/agent/hw/test:hw_copp_utils",
                "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
            ],
            versions = to_versions(sai_impl),
        )

def _sai_test_binary(sai_impl, is_npu):
    test_deps = [
        "//fboss/agent/hw/sai/hw_test:{}".format(get_switch_ensemble_name(sai_impl, is_npu)),
        "//fboss/agent/hw/test:hw_switch_test",
        "//fboss/agent/hw/test:hw_test_main",
        "//fboss/lib:ref_map",
        "//fboss/agent/hw/sai/impl:{}".format(to_impl_lib_name(sai_impl)),
        "//fboss/agent/hw/sai/hw_test:{}".format(get_switch_phy_capabilities_name(sai_impl, is_npu)),
    ]
    binary_name = "sai_test{}-{}-{}".format(impl_category_suffix(is_npu), sai_impl.name, sai_impl.version)
    return cpp_binary(
        name = binary_name,
        srcs = [
            "dataplane_tests/SaiAclTableGroupTrafficTests.cpp",
            "HwTestEcmpUtils.cpp",
            "HwTestFabricUtils.cpp",
            "HwTestNeighborUtils.cpp",
            "HwTestPtpTcUtils.cpp",
            "HwVlanUtils.cpp",
            "HwTestTamUtils.cpp",
            "HwTestAclUtils.cpp",
            "HwTestPfcUtils.cpp",
            "HwTestAqmUtils.cpp",
            "HwTestCoppUtils.cpp",
            "HwTestUdfUtils.cpp",
            "HwTestFlowletSwitchingUtils.cpp",
            "HwTestMirrorUtils.cpp",
            "HwTestMplsUtils.cpp",
            "HwTestPortUtils.cpp",
            "HwTestPacketTrapEntry.cpp",
            "HwTestRouteUtils.cpp",
            "HwTestTeFlowUtils.cpp",
            "HwTestTrunkUtils.cpp",
            "SaiAclTableTests.cpp",
            "SaiAclTableGroupTests.cpp",
            "SaiLinkStateRollbackTests.cpp",
            "SaiNeighborRollbackTests.cpp",
            "SaiNextHopGroupTest.cpp",
            "SaiPortUtils.cpp",
            "SaiPortAdminStateTests.cpp",
            "SaiRollbackTest.cpp",
            "SaiRouteRollbackTests.cpp",
            "SaiQPHRollbackTests.cpp",
        ],
        linker_flags = [
            "--export-dynamic",
            "--unresolved-symbols=ignore-all",
        ],
        deps = test_deps,
        link_group_map = get_link_group_map(binary_name, sai_impl),
        headers = [
            "SaiLinkStateDependentTests.h",
        ],
        versions = to_versions(sai_impl),
    )

def _sai_multinode_test_binary(sai_impl):
    test_deps = [
        "//fboss/agent/test:multinode_tests",
        "//fboss/agent:main-sai-{}".format(to_impl_lib_name(sai_impl)),
        "//fboss/agent/platforms/sai:{}".format(
            sai_switch_dependent_name("sai_platform", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:{}".format(
            sai_switch_dependent_name("sai_ecmp_utils", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:{}".format(
            sai_switch_dependent_name("sai_trunk_utils", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:{}".format(
            sai_switch_dependent_name("sai_copp_utils", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:{}".format(
            sai_switch_dependent_name("sai_acl_utils", sai_impl, True),
        ),
    ]
    if sai_impl.name == "fake" or sai_impl.name == "leaba":
        test_deps.append("//fboss/agent/platforms/sai:bcm-required-symbols")
    binary_name = "sai_multinode_test-{}-{}".format(sai_impl.name, sai_impl.version)
    return cpp_binary(
        name = binary_name,
        srcs = [
            "SaiMultiNodeTest.cpp",
        ],
        link_group_map = get_link_group_map(binary_name, sai_impl),
        deps = test_deps,
        auto_headers = AutoHeaders.SOURCES,
        versions = to_versions(sai_impl),
    )

def _sai_macsec_multinode_test_binary(sai_impl):
    test_deps = [
        "//fboss/agent/test:macsec_multinode_tests",
        "//fboss/agent:main-sai-{}".format(to_impl_lib_name(sai_impl)),
        "//fboss/agent/platforms/sai:{}".format(
            sai_switch_dependent_name("sai_platform", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:{}".format(
            sai_switch_dependent_name("sai_ecmp_utils", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:{}".format(
            sai_switch_dependent_name("sai_trunk_utils", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:{}".format(
            sai_switch_dependent_name("sai_copp_utils", sai_impl, True),
        ),
        "//fboss/agent/hw/sai/hw_test:{}".format(
            sai_switch_dependent_name("sai_acl_utils", sai_impl, True),
        ),
    ]
    if sai_impl.name == "fake" or sai_impl.name == "leaba":
        test_deps.append("//fboss/agent/platforms/sai:bcm-required-symbols")
    binary_name = "sai_macsec_multinode_test-{}-{}".format(sai_impl.name, sai_impl.version)
    return cpp_binary(
        name = binary_name,
        srcs = [
            "SaiMultiNodeTest.cpp",
        ],
        link_group_map = get_link_group_map(binary_name, sai_impl),
        deps = test_deps,
        auto_headers = AutoHeaders.SOURCES,
        versions = to_versions(sai_impl),
    )

def all_test_binaries():
    for sai_impl in get_all_impls_for(True):
        _sai_test_binary(sai_impl, True)
        _sai_multinode_test_binary(sai_impl)
        _sai_macsec_multinode_test_binary(sai_impl)

    for sai_impl in get_all_impls_for(False):
        _sai_test_binary(sai_impl, False)

def _test_handlers(is_npu):
    all_impls = get_all_impls_for(is_npu)
    for sai_impl in all_impls:
        _test_handler(sai_impl, is_npu)

def all_test_handlers():
    _test_handlers(is_npu = True)
    _test_handlers(is_npu = False)

def _test_handler(sai_impl, is_npu):
    return cpp_library(
        name = "{}".format(sai_switch_dependent_name("thrift_test_handler", sai_impl, is_npu)),
        srcs = [
            "SaiTestHandler.cpp",
        ],
        auto_headers = AutoHeaders.SOURCES,
        exported_deps = [
            ":if-cpp2-services",
            "//fboss/agent/hw/sai/diag:{}".format(sai_switch_dependent_name("diag_shell", sai_impl, is_npu)),
        ],
        versions = to_versions(sai_impl),
    )

def _test_thrift_handlers(is_npu):
    all_impls = get_all_impls_for(is_npu)
    for sai_impl in all_impls:
        _test_thrift_handler(sai_impl, is_npu)

def all_test_thrift_handlers():
    _test_thrift_handlers(is_npu = True)

def _test_thrift_handler(sai_impl, is_npu):
    switch_lib_name = sai_switch_lib_name(sai_impl, is_npu)
    return cpp_library(
        name = "{}".format(sai_switch_dependent_name("agent_hw_test_thrift_handler", sai_impl, is_npu)),
        srcs = [
            "HwTestAclUtilsThriftHandler.cpp",
            "HwTestMirrorUtilsThriftHandler.cpp",
            "HwTestNeighborUtilsThriftHandler.cpp",
            "HwTestEcmpUtilsThriftHandler.cpp",
            "HwTestPortUtilsThriftHandler.cpp",
            "HwTestVoqSwitchUtilsThriftHandler.cpp",
            "HwTestThriftHandler.cpp",
        ],
        auto_headers = AutoHeaders.SOURCES,
        exported_deps = [
            "//fboss/agent/test/utils:acl_test_utils",
            "//fboss/agent/hw/test:hw_test_thrift_handler_h",
            "//fboss/agent/if:agent_hw_test_ctrl-cpp2-services",
            "//fboss/agent/hw/sai/switch:{}".format(switch_lib_name),
            "//fboss/agent/hw/sai/hw_test:{}".format(
                sai_switch_dependent_name("sai_ecmp_utils", sai_impl, True),
            ),
            "//fboss/agent/hw/sai/diag:{}".format(sai_switch_dependent_name("diag_shell", sai_impl, is_npu)),
        ],
        versions = to_versions(sai_impl),
    )

def all_test_libs():
    all_acl_utils()
    all_ecmp_utils()
    all_fabric_utils()
    all_port_utils()
    all_packet_trap_helper()
    all_copp_utils()
    all_ptp_tc_utils()
    all_trunk_utils()
    all_udf_utils()
    all_teflow_utils()
    all_test_thrift_handlers()
