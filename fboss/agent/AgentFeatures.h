// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <gflags/gflags.h>

/*
 * Today:
 *  - Feature flag definitions (DEFINE_) are scattered across several files.
 *  - Other files that need to access the flag declare (DECLARE_) and use it.
 *
 *  However, that does not work if the file that defines a flag and the file
 *  that declares the flag are compiled into separate libraries with no shared
 *  dependency.
 *
 * AgentFeatures.{h, cpp} is intended to provide a clean way to solve this:
 *
 * agent_features library:
 *    - All flags will be DEFINE_'d in AgentFeatures.cpp
 *    - Each of these flags will be DECLARE_'d in AgentFeatures.h
 *    - This is compiled in library agent_features.
 * To use a flag in an Agent library:
 *    - include AgentFeatures.h
 *    - add agent_features to dep
 *    - use the flag
 *
 * Going forward, we will add new flags to this file.
 *
 * TODO: move existing flags to this file.
 */

DECLARE_bool(dsf_4k);
DECLARE_bool(dsf_100g_nif_breakout);
DECLARE_bool(enable_acl_table_chain_group);
DECLARE_int32(oper_sync_req_timeout);
DECLARE_bool(hide_fabric_ports);

DECLARE_bool(dsf_subscribe);
DECLARE_bool(dsf_subscriber_skip_hw_writes);
DECLARE_bool(dsf_subscriber_cache_updated_state);
DECLARE_uint32(dsf_gr_hold_time);
DECLARE_bool(dsf_flush_remote_sysports_and_rifs_on_gr);
DECLARE_uint32(dsf_num_parallel_sessions_per_remote_interface_node);
DECLARE_int32(dsf_num_fsdb_connect_threads);
DECLARE_int32(dsf_num_fsdb_stream_threads);
DECLARE_bool(dsf_subscribe_patch);
DECLARE_int32(dsf_subscriber_reconnect_thread_heartbeat_ms);
DECLARE_int32(dsf_subscriber_stream_thread_heartbeat_ms);

DECLARE_bool(set_classid_for_my_subnet_and_ip_routes);
DECLARE_int32(stat_publish_interval_ms);
DECLARE_int32(hwagent_port_base);
DECLARE_bool(force_init_fp);
DECLARE_bool(flowletSwitchingEnable);
DECLARE_bool(dlbResourceCheckEnable);
DECLARE_bool(disable_neighbor_solicitation);
DECLARE_bool(disable_looped_fabric_ports);
DECLARE_bool(detect_wrong_fabric_connections);
DECLARE_bool(dsf_edsw_platform_mapping);
DECLARE_bool(exit_for_any_hw_disconnect);
DECLARE_bool(enable_balanced_intput_mode);
DECLARE_int32(hw_agent_connection_timeout_ms);
