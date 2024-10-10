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

DEFINE_bool(
    enable_balanced_intput_mode,
    true,
    "Enable balanced input mode on fabric devices");

DEFINE_int32(
    hw_agent_connection_timeout_ms,
    0,
    "Time to wait for HwSwitch to connect before SwSwitch exits. "
    "By default, SwSwitch waits forever and hence default value is 0.");
