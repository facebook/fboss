# Copyright (c) 2004-present, Facebook, Inc.

load("@fbcode_macros//build_defs:create_build_mode.bzl", "create_build_mode")

_extra_asan_options = {
    "detect_leaks": "1",
}

_lsan_suppressions = [
    # Suppress leaks in third-party BRCM libraries for now.
    # TODO: work with BRCM to fix these leaks in their code.
    "bcm_switch_event_register",
    "_bcm_attach",
    "soc_attach",
    "soc_do_init",
    "soc_misc_init",
    "sal_appl_init",
    # field_group_sync gets called during warm boot exit
    # Practically this leak is not a big deal since we are
    # going to exit the process anyways
    "_field_group_sync",
    # CSP CS9371557 tracks this leak with Broadcom
    "bcm_trunk_member_add",
    "bcm_trunk_member_delete",
    # CSP CS9690891 tracks this leak with Broadcom
    "bcm_mpls_tunnel_initiator_set",
    # CS9737327
    "_brcm_sai_switch_flex_counter_init",
    # CS10319636
    "syncdbUtilSchemaCreate",
    # On exit, bcm sdk code asks rx thread
    # to stop and waits 0.5s for thread to
    # stop. If it does not, exit continues
    # anyways. This then causes memory allocated
    # in rx_start to be leaked causing test
    # failures
    "_bcm_common_rx_start",
    # CS00010980235
    "_brcm_sai_add_port_to_default_vlan",
    "_brcm_sai_vlan_init",
    "_brcm_sai_list_init_end",
    "la_acl_delegate",
    # CS00011422528
    "sai_driver_shell",
    # CS00011825929
    "_bcm_esw_stat_flex_update_ingress_flex_info",
    # CS00012268611
    "_bcm_esw_stat_group_mode_id_config_create",
    # TODO  Remove the below as part of T143911621
    # Disable asan warning till DLB warmboot support implemented
    "bcm_th2_l3_egress_dlb_attr_set",
    "CRYPTO_zalloc",
    # Suppress EDK init memleaks until BRCM fixes the leak
    # CSP CS00012369114
    "devicemgr_open",
    "devicemgr_alloc_resources",
    "sharedheap_open",
    "rpc_open",
]

_tsan_suppressions = [
    # Benign lock-order-inversion warning. Rib is constructed from HwSwitch::init
    # while holding the HwSwitch lock. During construction we acquire the rib lock.
    # During state updates we hold the RIB lock and acquire HwSwitch lock. TSAN
    # now (rightly) complains about lock order inversion. However this inversion
    # is benign since the first lock sequence is during init where no state updates
    # are allowed
    "deadlock:fboss::RoutingInformationBase::fromFollyDynamic",

    # libnss_flare.so is dynamically linked (and isn't built with tsan),
    # so it doesn't seem that the in-code suppressions work for it.
    # This causes us to have TSAN failures, even when the offending line is
    # supposed to have a TSAN suppression: https://fburl.com/code/jye4t32e
    "race:libnss_flare.so",
]
_extra_warnings = [
    "-Winconsistent-missing-override",
    "-Wsuggest-override",
]

_extra_cflags = [
    "-fno-stack-protector",
]

_extra_cxxflags = [
    "-fno-stack-protector",
]

_mode = create_build_mode(
    asan_options = _extra_asan_options,
    lsan_suppressions = _lsan_suppressions,
    tsan_suppressions = _tsan_suppressions,
    gcc_flags = _extra_warnings,
    clang_flags = _extra_warnings,
    c_flags = _extra_cflags,
    cxx_flags = _extra_cxxflags,
)

_modes = {
    "dbg": _mode,
    "dbgo": _mode,
    "dev": _mode,
    "opt": _mode,
}

def get_modes():
    """ Return modes for this hierarchy """
    return _modes
