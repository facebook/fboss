#!/usr/bin/env python3

# pyre-unsafe

# Run as python3 run_bcm_cint_audit.py
#
#
# This script looks for brcm APIs that are currently not logged by Bcm Cinter.
# It works as follows:
#   - recursively traverse to find all lines under agent/hw that matches
#     the bcm API regex.
#   - remove the duplicate ones
#   - diff with wrapped symbols (APIs logged by Bcm Cinter) and print output.

import subprocess
import sys

ROOT_DIR_TO_SEARCH = "fboss/agent/hw/bcm"
PATTERNS_TO_SEARCH = [r"'bcm_\S*[a-z,A-Z,0-9]('"]
PATTENRS_TO_OMIT = [
    "SdkTracer",
    "BcmSdkInterface",
    "BcmCinter",
    "BcmInterface",
    "TARGETS",
    "bzl",
]
PATTERNS_TO_REPLACE = ["'s/^.*bcm_/bcm_/'", "'s/(.*$//'"]
PATTERNS_TO_OMIT_AFTER_REPLACEMENT = [
    r"'\.'",
    "'_t[*,(,),<,>,$]'",
    "_get",
    "_list",
    "_traverse",
]

WRAPPED_SYMBOLS = "fboss/agent/hw/bcm/wrapped_symbols.bzl"
WRAPPED_SYMBOLS_TO_SEARCH = [
    "'wrap=" + search_pattern[1:-2] + "'" for search_pattern in PATTERNS_TO_SEARCH
]  # Remove trailing close bracket
WRAPPED_SYMBOLS_TO_REPLACE = ["'s/.*--wrap=//'", "'s/\",//'"]

KNOWN_MISSING_APIS = {
    # QCM-related APIs. We don't need to wrap these
    # since they are not supported after 6.5.17.
    "bcm_cosq_burst_monitor_detach",
    "bcm_cosq_burst_monitor_flow_view_config_set",
    "bcm_cosq_burst_monitor_init",
    "bcm_cosq_burst_monitor_set",
    "bcm_field_entry_policer_attach",
    "bcm_field_entry_policer_detach",
    "bcm_field_group_oper_mode_set",
    "bcm_field_qualify_ExactMatchHitStatus",
    "bcm_flowtracker_export_template_create",
    "bcm_flowtracker_export_template_destroy",
    "bcm_flowtracker_group_age_timer_set",
    "bcm_flowtracker_group_collector_attach",
    "bcm_flowtracker_group_collector_detach",
    "bcm_flowtracker_group_create",
    "bcm_flowtracker_group_destroy",
    "bcm_flowtracker_group_export_trigger_set",
    "bcm_flowtracker_group_flow_limit_set",
    "bcm_flowtracker_group_info_t_init",
    "bcm_flowtracker_group_tracking_params_set",
    "bcm_flowtracker_tracking_param_info_t_init",
    "bcm_policer_config_t_init",
    "bcm_policer_create",
    "bcm_policer_destroy",
    # End of Qcm related APIs.
    "bcm_errmsg",
    "bcm_field_group_config_t_init",
    "bcm_l2_addr_delete_by_mac_port",
    "bcm_l2_addr_register",
    "bcm_l2_addr_t_init",
    "bcm_l2_addr_unregister",
    "bcm_l2_station_t_init",
    "bcm_l3_ecmp_member_t_init",
    "bcm_l3_host_t_init",
    "bcm_l3_info_t_init",
    "bcm_l3_ingress_t_init",
    "bcm_l3_intf_t_init",
    "bcm_l3_route_t_init",
    "bcm_mirror_destination_t_init",
    "bcm_mpls_egress_label_t_init",
    "bcm_mpls_tunnel_switch_t_init",
    "bcm_pkt_rx_free",
    "bcm_pktio_alloc",
    "bcm_pktio_cleanup",
    "bcm_pktio_free",
    "bcm_pktio_init",
    "bcm_pktio_pmd_set",
    "bcm_pktio_pull",
    "bcm_pktio_put",
    "bcm_pktio_rx_register",
    "bcm_pktio_rx_unregister",
    "bcm_pktio_trim",
    "bcm_pktio_tx",
    "bcm_port_control_phy_timesync_set",
    "bcm_port_flood_block_set",
    "bcm_port_if_t",
    "bcm_port_phy_set",
    "bcm_port_phy_tx_t_init",
    "bcm_port_priority_group_config_t_init",
    "bcm_port_resource_t_init",
    "bcm_qos_map_t_init",
    "bcm_rtag7_control_set",
    "bcm_rx_cosq_mapping_t_init",
    "bcm_rx_queue_channel_set",
    "bcm_rx_reasons_t_init",
    "bcm_shutdown",
    "bcm_stat_sync",
    "bcm_trunk_info_t_init",
    "bcm_trunk_member_t_init",
    "bcm_vlan_control_vlan_t_init",
    "bcm_warmboot_set",
    # init calls don't need to be wrapped.
    "bcm_cosq_pfc_class_map_config_t_init",
    "bcm_field_entry_config_t_init",
    "bcm_flexctr_action_t_init",
    "bcm_stat_group_mode_attr_selector_t_init",
    "bcm_port_fdr_config_t_init",
}


def get_used_apis():
    command = "("
    # search all the files
    for search_pattern in PATTERNS_TO_SEARCH:
        command = command + f"grep -r {search_pattern} {ROOT_DIR_TO_SEARCH};"
    command += ")"

    # omit certain patterns
    for omit_pattern in PATTENRS_TO_OMIT:
        command += f" | grep -v {omit_pattern}"

    # strip the line
    for replace_pattern in PATTERNS_TO_REPLACE:
        command += f" | sed {replace_pattern}"

    # omit some other patterns after replacement
    for omit_pattern_after_replacement in PATTERNS_TO_OMIT_AFTER_REPLACEMENT:
        command += f" | grep -v {omit_pattern_after_replacement}"

    return set(
        subprocess.run(command, shell=True, capture_output=True)
        .stdout.decode("utf-8")
        .split("\n")[:-1]
    )


def get_wrapped_apis():
    command = "("
    # search in wrapped_symbols.bzl
    for search_pattern in WRAPPED_SYMBOLS_TO_SEARCH:
        command = command + f"grep {search_pattern} {WRAPPED_SYMBOLS};"
    command += ")"

    # strip the line
    for replace_pattern in WRAPPED_SYMBOLS_TO_REPLACE:
        command += f" | sed {replace_pattern}"

    return set(
        subprocess.run(command, shell=True, capture_output=True)
        .stdout.decode("utf-8")
        .split("\n")[:-1]
    )


def main() -> None:
    used_apis = get_used_apis()
    wrapped_apis = get_wrapped_apis()
    diff = used_apis - wrapped_apis - KNOWN_MISSING_APIS
    if len(diff) > 0:
        linking_verb = "is" if len(diff) == 1 else "are"
        print(
            f"There {linking_verb} {len(diff)} Bcm API(s) not supported by Bcm Cinter.\n"
            "Please make sure to log the API in Bcm Cinter when introducing new Bcm APIs.\n"
        )
        print(*sorted(diff), sep="\n")
        sys.exit(1)
    else:
        print("No missing API in Bcm Cinter.")


if __name__ == "__main__":
    main()
