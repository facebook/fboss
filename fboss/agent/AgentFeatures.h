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
DECLARE_bool(hide_management_ports);

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
DECLARE_int32(dsf_session_conn_timeout_ms);
DECLARE_int32(dsf_session_recv_timeout_ms);

DECLARE_bool(set_classid_for_my_subnet_and_ip_routes);
DECLARE_int32(stat_publish_interval_ms);
DECLARE_int32(hwagent_port_base);
DECLARE_bool(force_init_fp);
DECLARE_bool(flowletSwitchingEnable);
DECLARE_bool(enable_ecmp_random_spray);
DECLARE_bool(dlbResourceCheckEnable);
DECLARE_bool(use_full_dlb_scale);
DECLARE_bool(disable_neighbor_solicitation);
DECLARE_bool(disable_looped_fabric_ports);
DECLARE_bool(detect_wrong_fabric_connections);
DECLARE_bool(dsf_edsw_platform_mapping);
DECLARE_bool(exit_for_any_hw_disconnect);
DECLARE_bool(enable_balanced_input_mode);
DECLARE_int32(hw_agent_connection_timeout_ms);
DECLARE_bool(qgroup_guarantee_enable);
DECLARE_bool(skip_buffer_reservation);
DECLARE_bool(fix_lossless_mode_per_pg);
DECLARE_int32(fboss_event_base_queue_limit);
DECLARE_bool(dual_stage_rdsw_3q_2q);
DECLARE_bool(dual_stage_edsw_3q_2q);
DECLARE_bool(dual_stage_3q_2q_qos);
DECLARE_bool(janga_test);

bool isDualStage3Q2QMode();
bool isDualStage3Q2QQos();
DECLARE_bool(enable_hw_update_protection);
DECLARE_int32(max_l2_entries);
DECLARE_int32(max_ndp_entries);
DECLARE_int32(max_arp_entries);
DECLARE_bool(enforce_resource_hw_limits);

DECLARE_int32(neighbhor_resource_percentage);
DECLARE_bool(enable_route_resource_protection);
DECLARE_int32(max_mac_address_to_block);
DECLARE_int32(max_neighbors_to_block);

DECLARE_bool(link_stress_test);
DECLARE_int32(ecmp_resource_percentage);
DECLARE_int32(ars_resource_percentage);
DECLARE_int32(switch_index_for_testing);
DECLARE_uint32(counter_refresh_interval);

DECLARE_bool(run_forever);
DECLARE_bool(run_forever_on_failure);

DECLARE_string(sdk_reg_dump_path_prefix);
DECLARE_bool(type_dctype1_janga);
DECLARE_bool(prod_invariant_config_test);
DECLARE_int32(max_unprocessed_switch_reachability_changes);
DECLARE_bool(enable_ecmp_resource_manager);
DECLARE_int32(ecmp_resource_manager_make_before_break_buffer);

DECLARE_int32(update_stats_interval_s);
DECLARE_bool(update_route_with_dlb_type);
DECLARE_int32(agent_exit_delay_s);

DECLARE_bool(dsf_single_stage_r192_f40_e32);

DECLARE_bool(enable_high_frequency_stats_polling);
DECLARE_bool(dsf_headroom_pool_size_multiplication_factor_fix);
DECLARE_bool(ignore_asic_hard_reset_notification);
DECLARE_bool(cleanup_probed_kernel_data);
DECLARE_bool(ndp_static_neighbor);
DECLARE_bool(dsf_single_stage_r128_f40_e16_8k_sys_ports);
DECLARE_bool(dsf_single_stage_r128_f40_e16_uniform_local_offset);
DECLARE_bool(hyper_port);
DECLARE_uint32(ecmp_width);
DECLARE_bool(enable_th5_ars_scale_mode);
DECLARE_bool(check_wb_handles);
DECLARE_bool(enable_fabric_link_monitoring);
DECLARE_bool(lldp_port_drain_state);
DECLARE_bool(enable_state_delta_logging);
DECLARE_bool(strip_vlan_for_pipeline_bypass);
DECLARE_string(state_delta_log_file);
DECLARE_string(state_delta_log_protocol);
DECLARE_int32(state_delta_log_timeout_ms);
DECLARE_int32(fsdbStatsStreamIntervalSeconds);
DECLARE_bool(recover_from_hw_switch);
