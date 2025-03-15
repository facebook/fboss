// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/AgentFeatures.h"

DEFINE_bool(dsf_4k, false, "Enable DSF Scale Test config");

DEFINE_bool(dsf_100g_nif_breakout, false, "Enable J3 DSF Scale Test config");

DEFINE_bool(
    sai_user_defined_trap,
    false,
    "Flag to use user defined trap when programming ACL action to punt packets to cpu queue.");

DEFINE_bool(enable_acl_table_chain_group, false, "Allow ACL table chaining");

DEFINE_int32(
    oper_sync_req_timeout,
    30,
    "request timeout for oper sync client in seconds");

DEFINE_bool(
    classid_for_unresolved_routes,
    false,
    "Flag to set the class ID for unresolved routes that points  to CPU port");

DEFINE_bool(hide_fabric_ports, false, "Elide ports of type fabric");

// DSF Subscriber flags
DEFINE_bool(
    dsf_subscribe,
    true,
    "Issue DSF subscriptions to all DSF Interface nodes");
DEFINE_bool(dsf_subscriber_skip_hw_writes, false, "Skip writing to HW");
DEFINE_bool(
    dsf_subscriber_cache_updated_state,
    false,
    "Cache switch state after update by dsf subsriber");
DEFINE_uint32(
    dsf_gr_hold_time,
    0,
    "GR hold time for FSDB DsfSubscription in sec");
DEFINE_bool(
    dsf_subscribe_patch,
    false,
    "Subscribe to remote FSDB using Patch apis");
DEFINE_int32(
    dsf_subscriber_reconnect_thread_heartbeat_ms,
    1000,
    "DSF subscriber reconnect thread heartbeat interval in msec");
DEFINE_int32(
    dsf_subscriber_stream_thread_heartbeat_ms,
    1000,
    "DSF subscriber stream thread heartbeat interval in msec");
// Remote neighbor entries are always flushed to avoid blackholing the traffic.
// However, by default, remote{systemPorts, Rifs} are not flushed but marked
// STALE in the software. This is to avoid hardware programmign churn.
// Setting this flag to True will cause Agent to flush remote{systemPorts,
// Rifs} from the hardware.
DEFINE_bool(
    dsf_flush_remote_sysports_and_rifs_on_gr,
    false,
    "Flush Remote{systemPorts, Rifs} on GR");
DEFINE_uint32(
    dsf_num_parallel_sessions_per_remote_interface_node,
    1,
    "Number of parallel DSF sessions per remote Interface Node. "
    "1 for Prod. > 1 for scale tests");

DEFINE_int32(
    dsf_num_fsdb_connect_threads,
    1,
    "Number of threads to use for DSF remote connection pool");

DEFINE_int32(
    dsf_num_fsdb_stream_threads,
    1,
    "Number of threads to use for DSF remote stream pool");

DEFINE_int32(
    dsf_session_conn_timeout_ms,
    4000,
    "Socket connection timeout for DSF session");

DEFINE_int32(
    dsf_session_recv_timeout_ms,
    12000,
    "Socket pkt receive timeout for DSF session");

DEFINE_bool(
    set_classid_for_my_subnet_and_ip_routes,
    false,
    "Flag to disable implicit route classid set by sai/sdk, and always explicitly set class ID for my subnet routes and my ip routes from fboss");

DEFINE_int32(
    stat_publish_interval_ms,
    1000,
    "How frequently to publish thread-local stats back to the "
    "global store.  This should generally be less than 1 second.");

DEFINE_int32(
    hwagent_port_base,
    5931,
    "The first thrift server port reserved for HwAgent");

DEFINE_bool(force_init_fp, true, "Force full field processor initialization");

DEFINE_bool(
    flowletSwitchingEnable,
    false,
    "Flag to turn on flowlet switching for DLB");

// TODO (ravi)
// This is more a safety tool for fast rollback if RTSWs run into an issue
DEFINE_bool(
    dlbResourceCheckEnable,
    true,
    "Flag to enable resource checks on DLB ecmp groups");

DEFINE_bool(
    send_icmp_time_exceeded,
    true,
    "Flag to indicate whether to send ICMP time exceeded for hop limit exceeded");

DEFINE_bool(
    disable_looped_fabric_ports,
    true,
    "Disable fabric ports where loop is detected to stop traffic blackholing");

// Wrong fabric connection detection. Flag to enable/disable this mechanism in
// SDK
DEFINE_bool(
    detect_wrong_fabric_connections,
    true,
    "Enable wrong fabric connection. Done via SDK");

DEFINE_bool(dsf_edsw_platform_mapping, false, "Use EDSW platform mapping");

DEFINE_bool(
    exit_for_any_hw_disconnect,
    false,
    "Flag to indicate whether SwSwitch will crash if any hw switch connection is lost. This will be used in tests to ensure all hw agent running.");

// TODO: Need fix for the feature on single link configuration (CS00012375262)
DEFINE_bool(
    enable_balanced_input_mode,
    false,
    "Enable balanced input mode on fabric devices");

DEFINE_int32(
    hw_agent_connection_timeout_ms,
    0,
    "Time to wait for HwSwitch to connect before SwSwitch exits. "
    "By default, SwSwitch waits forever and hence default value is 0.");

DEFINE_bool(
    qgroup_guarantee_enable,
    false,
    "Enable setting of unicast and multicast queue guaranteed buffer sizes");

DEFINE_bool(skip_buffer_reservation, false, "Enable skip reservation");

// TODO(zecheng): Remove this once firmware support is ready
DEFINE_bool(
    conditional_entropy_cpu_seed_test_only,
    false,
    "Enable test-only feature for CPU updating conditional entropy seed");

DEFINE_bool(
    fix_lossless_mode_per_pg,
    false,
    "Flag to disruptively update lossless mode per pg");

DEFINE_int32(fboss_event_base_queue_limit, 10000, "FbossEventBase queue limit");

DEFINE_bool(
    dual_stage_rdsw_3q_2q,
    false,
    "Use platform mapping for dual stage RDSW with 3q and 2q model");

DEFINE_bool(
    dual_stage_edsw_3q_2q,
    false,
    "Use platform mapping for dual stage EDSW with 3q and 2q model");

DEFINE_bool(
    dual_stage_3q_2q_qos,
    false,
    "Use qos setting for dual stage 3q and 2q model");

bool isDualStage3Q2QMode() {
  return FLAGS_dual_stage_rdsw_3q_2q || FLAGS_dual_stage_edsw_3q_2q;
}

bool isDualStage3Q2QQos() {
  return isDualStage3Q2QMode() || FLAGS_dual_stage_3q_2q_qos;
}

DEFINE_bool(
    enable_hw_update_protection,
    false,
    "Enable Neighbor/MAC table hw update failure protection");

DEFINE_bool(
    fw_drained_unrecoverable_error,
    false,
    "Enable or disable whether firmware drained(isolation) can be unrecoverable error");

DEFINE_int32(
    neighbhor_resource_percentage,
    75,
    "Percentage of neighbor resources (out of 100) allowed to use before ResourceAccountant rejects the update.");

DEFINE_bool(
    enable_route_resource_protection,
    true,
    "Enable route resource protection for Resource Accountant");

DEFINE_int32(
    max_mac_address_to_block,
    10000,
    "Max number of mac addresses to block");

DEFINE_int32(
    max_neighbors_to_block,
    10000,
    "Max number of neighbor entries to block");

DEFINE_bool(
    link_stress_test,
    false,
    "enable to run stress tests (longer duration + more iterations)");

DEFINE_int32(
    ecmp_resource_percentage,
    75,
    "Percentage of ECMP resources (out of 100) allowed to use before ResourceAccountant rejects the update.");

DEFINE_int32(
    switch_index_for_testing,
    0,
    "switch index under test. Used for testing NPU specific features.");

DEFINE_uint32(
    counter_refresh_interval,
    1,
    "Counter refresh interval in seconds. Set it to 0 to fetch stats from HW");

DEFINE_bool(run_forever, false, "run the test forever");
DEFINE_bool(run_forever_on_failure, false, "run the test forever on failure");

DEFINE_string(
    sdk_reg_dump_path_prefix,
    "/var/facebook/logs/fboss/sdk/reg_dump",
    "File path prefix for SDK register dump");
