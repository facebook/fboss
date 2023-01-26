/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <folly/File.h>
#include <folly/Synchronized.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AsyncLogger.h"
#include "fboss/agent/hw/bcm/BcmInterface.h"
#include "fboss/agent/hw/bcm/BcmSdkInterface.h"

extern "C" {
#include <bcm/cosq.h>
#include <bcm/field.h>
}

namespace facebook::fboss {

class BcmCinter : public BcmSdkInterface, public BcmInterface {
 public:
  explicit BcmCinter();
  ~BcmCinter() override;
  static std::shared_ptr<BcmCinter> getInstance();

  /*
   * TODO: bcm_rx_*
   */
  int bcm_rx_stop(int /*unit*/, bcm_rx_cfg_t* /*cfg*/) override {
    return 0;
  }
  int bcm_rx_start(int /*unit*/, bcm_rx_cfg_t* /*cfg*/) override {
    return 0;
  }
  int bcm_rx_free(int /*unit*/, void* /*pkt_data*/) override {
    return 0;
  }
  int bcm_rx_control_set(int /*unit*/, bcm_rx_control_t /*type*/, int /*arg*/)
      override {
    return 0;
  }
  int bcm_rx_register(
      int /*unit*/,
      const char* /*name*/,
      bcm_rx_cb_f /*callback*/,
      uint8 /*priority*/,
      void* /*cookie*/,
      uint32 /*flags*/) override {
    return 0;
  }
  int bcm_rx_unregister(
      int /*unit*/,
      bcm_rx_cb_f /*callback*/,
      uint8 /*priority*/) override {
    return 0;
  }

  /*
   * Unimplemeted - Our application does not use these APIs currently.
   */
  int bcm_port_ability_advert_set(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_port_ability_t* /*ability_mask*/) override {
    XLOG(WARN) << __func__ << " unimplemented in Bcm Cinter";
    return 0;
  }
  int bcm_knet_filter_create(int /*unit*/, bcm_knet_filter_t* /*filter*/)
      override {
    XLOG(WARN) << __func__ << " unimplemented in Bcm Cinter";
    return 0;
  }
  int bcm_knet_netif_create(int /*unit*/, bcm_knet_netif_t* /*netif*/)
      override {
    XLOG(WARN) << __func__ << " unimplemented in Bcm Cinter";
    return 0;
  }
  int bcm_knet_netif_destroy(int /*unit*/, int /*netif_id*/) override {
    XLOG(WARN) << __func__ << " unimplemented in Bcm Cinter";
    return 0;
  }
  int bcm_knet_filter_destroy(int /*unit*/, int /*filter_id*/) override {
    XLOG(WARN) << __func__ << " unimplemented in Bcm Cinter";
    return 0;
  }
  int bcm_stg_list_destroy(int /*unit*/, bcm_stg_t* /*list*/, int /*count*/)
      override {
    XLOG(WARN) << __func__ << " unimplemented in Bcm Cinter";
    return 0;
  }
  int bcm_port_selective_set(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_port_info_t* /*info*/) override {
    XLOG(WARN) << __func__ << " unimplemented in Bcm Cinter";
    return 0;
  }

  /*
   * Implemented APIs
   */
  int bcm_field_qualify_Ttl(
      int unit,
      bcm_field_entry_t entry,
      uint8 data,
      uint8 mask) override;
  int bcm_field_qualify_IpType(
      int unit,
      bcm_field_entry_t entry,
      bcm_field_IpType_t type) override;
  int bcm_field_qualify_DSCP(
      int unit,
      bcm_field_entry_t entry,
      uint8 data,
      uint8 mask) override;
  int bcm_field_qualify_DstClassL2(
      int unit,
      bcm_field_entry_t entry,
      uint32 data,
      uint32 mask) override;
  int bcm_field_qualify_DstClassL3(
      int unit,
      bcm_field_entry_t entry,
      uint32 data,
      uint32 mask) override;
  int bcm_field_qualify_PacketRes(
      int unit,
      bcm_field_entry_t entry,
      uint32 data,
      uint32 mask) override;
  int bcm_field_qualify_OuterVlanId(
      int unit,
      bcm_field_entry_t entry,
      bcm_vlan_t data,
      bcm_vlan_t mask) override;
  int bcm_field_qualify_EtherType(
      int unit,
      bcm_field_entry_t entry,
      uint16 data,
      uint16 mask) override;
  int bcm_field_action_delete(
      int unit,
      bcm_field_entry_t entry,
      bcm_field_action_t action,
      uint32 param0,
      uint32 param1) override;
  int bcm_port_loopback_set(int unit, bcm_port_t port, uint32 value) override;
  int bcm_udf_hash_config_add(
      int unit,
      uint32 options,
      bcm_udf_hash_config_t* config) override;
  int bcm_udf_hash_config_delete(int unit, bcm_udf_hash_config_t* config)
      override;
  int bcm_udf_create(
      int unit,
      bcm_udf_alloc_hints_t* hints,
      bcm_udf_t* udf_info,
      bcm_udf_id_t* udf_id) override;
  int bcm_udf_destroy(int unit, bcm_udf_id_t udf_id) override;
  int bcm_udf_pkt_format_create(
      int unit,
      bcm_udf_pkt_format_options_t options,
      bcm_udf_pkt_format_info_t* pkt_format,
      bcm_udf_pkt_format_id_t* pkt_format_id) override;
  int bcm_udf_pkt_format_destroy(
      int unit,
      bcm_udf_pkt_format_id_t pkt_format_id) override;
  int bcm_udf_pkt_format_add(
      int unit,
      bcm_udf_id_t udf_id,
      bcm_udf_pkt_format_id_t pkt_format_id) override;
  int bcm_udf_pkt_format_delete(
      int unit,
      bcm_udf_id_t udf_id,
      bcm_udf_pkt_format_id_t pkt_format_id) override;
  int bcm_udf_pkt_format_get(
      int /*unit*/,
      bcm_udf_pkt_format_id_t /*pkt_format_id*/,
      int /*max*/,
      bcm_udf_id_t* /*udf_id_list*/,
      int* /*actual*/) override {
    return 0;
  }
  int bcm_port_pause_addr_set(int unit, bcm_port_t port, bcm_mac_t mac)
      override;

  int bcm_udf_hash_config_get(int /*unit*/, bcm_udf_hash_config_t* /*config*/)
      override {
    return 0;
  }
  int bcm_udf_pkt_format_info_get(
      int /*unit*/,
      bcm_udf_pkt_format_id_t /*pkt_format_id*/,
      bcm_udf_pkt_format_info_t* /*pkt_format*/) override {
    return 0;
  }
  void bcm_udf_pkt_format_info_t_init(
      bcm_udf_pkt_format_info_t* pkt_format) override;

  void bcm_udf_alloc_hints_t_init(bcm_udf_alloc_hints_t* udf_hints) override;
  void bcm_udf_t_init(bcm_udf_t* udf_info) override;
  void bcm_udf_hash_config_t_init(bcm_udf_hash_config_t* config) override;
  int bcm_udf_init(int unit) override;
  int bcm_udf_get(
      int /*unit*/,
      bcm_udf_id_t /*udf_id*/,
      bcm_udf_t* /*udf_info*/) override {
    return 0;
  }
  int bcm_port_autoneg_set(int unit, bcm_port_t port, int autoneg) override;
  int bcm_port_phy_modify(
      int unit,
      bcm_port_t port,
      uint32 flags,
      uint32 phy_reg_addr,
      uint32 phy_data,
      uint32 phy_mask) override;
  int bcm_port_dtag_mode_set(int unit, bcm_port_t port, int mode) override;
  int bcm_cosq_bst_stat_clear(
      int unit,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq,
      bcm_bst_stat_id_t bid) override;
  int bcm_field_entry_create_id(
      int unit,
      bcm_field_group_t group,
      bcm_field_entry_t entry) override;
  int bcm_cosq_bst_profile_set(
      int unit,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq,
      bcm_bst_stat_id_t bid,
      bcm_cosq_bst_profile_t* profile) override;
  int bcm_cosq_gport_discard_set(
      int unit,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq,
      bcm_cosq_gport_discard_t* discard) override;
  int bcm_cosq_gport_mapping_set(
      int unit,
      bcm_gport_t ing_port,
      bcm_cos_t priority,
      uint32_t flags,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq) override;
  int bcm_field_range_create(
      int unit,
      bcm_field_range_t* range,
      uint32 flags,
      bcm_l4_port_t min,
      bcm_l4_port_t max) override;
  int bcm_field_range_destroy(int unit, bcm_field_range_t range) override;
  int bcm_qos_map_create(int unit, uint32 flags, int* map_id) override;
  int bcm_qos_map_destroy(int unit, int map_id) override;
  int bcm_qos_map_add(int unit, uint32 flags, bcm_qos_map_t* map, int map_id)
      override;
  int bcm_qos_map_delete(int unit, uint32 flags, bcm_qos_map_t* map, int map_id)
      override;
  int bcm_qos_port_map_set(
      int unit,
      bcm_gport_t gport,
      int ing_map,
      int egr_map) override;
  int bcm_port_dscp_map_mode_set(int unit, bcm_port_t port, int mode) override;
  int bcm_field_qualify_IcmpTypeCode(
      int unit,
      bcm_field_entry_t entry,
      uint16 data,
      uint16 mask) override;
  int bcm_switch_control_set(int unit, bcm_switch_control_t type, int arg)
      override;
  int bcm_l3_egress_ecmp_ethertype_set(
      int unit,
      uint32 flags,
      int ethertype_count,
      int* ethertype_array) override;
  int bcm_l3_egress_ecmp_member_status_set(int unit, bcm_if_t intf, int status)
      override;
  int bcm_switch_control_port_set(
      int unit,
      bcm_port_t port,
      bcm_switch_control_t type,
      int arg) override;
  int bcm_field_qualify_SrcPort(
      int unit,
      bcm_field_entry_t entry,
      bcm_module_t data_modid,
      bcm_module_t mask_modid,
      bcm_port_t data_port,
      bcm_port_t mask_port) override;
  int bcm_field_qualify_RangeCheck(
      int unit,
      bcm_field_entry_t entry,
      bcm_field_range_t range,
      int invert) override;
  int bcm_field_qualify_DstPort(
      int unit,
      bcm_field_entry_t entry,
      bcm_module_t data_modid,
      bcm_module_t mask_modid,
      bcm_port_t data_port,
      bcm_port_t mask_port) override;
  int bcm_field_qualify_TcpControl(
      int unit,
      bcm_field_entry_t entry,
      uint8 data,
      uint8 mask) override;
  int bcm_rx_cosq_mapping_set(
      int unit,
      int index,
      bcm_rx_reasons_t reasons,
      bcm_rx_reasons_t reasons_mask,
      uint8 int_prio,
      uint8 int_prio_mask,
      uint32 packet_type,
      uint32 packet_type_mask,
      bcm_cos_queue_t cosq) override;
  int bcm_rx_cosq_mapping_delete(int unit, int index) override;
  std::vector<std::string> cintForRxCosqMapping(bcm_rx_cosq_mapping_t* cosqMap);
  int bcm_rx_cosq_mapping_extended_add(
      int unit,
      int options,
      bcm_rx_cosq_mapping_t* cosqMap) override;
  int bcm_rx_cosq_mapping_extended_delete(
      int unit,
      bcm_rx_cosq_mapping_t* cosqMap) override;
  int bcm_rx_cosq_mapping_extended_set(
      int unit,
      uint32 options,
      bcm_rx_cosq_mapping_t* cosqMap) override;
  int bcm_field_qualify_IpFrag(
      int unit,
      bcm_field_entry_t entry,
      bcm_field_IpFrag_t frag_info) override;
  int bcm_field_qualify_IpProtocol(
      int unit,
      bcm_field_entry_t entry,
      uint8 data,
      uint8 mask) override;
  int bcm_port_pause_sym_set(int unit, bcm_port_t port, int pause) override;
  int bcm_port_pause_set(int unit, bcm_port_t port, int pause_tx, int pause_rx)
      override;
  int bcm_port_sample_rate_set(
      int unit,
      bcm_port_t port,
      int ingress_rate,
      int egress_rate) override;
  int bcm_port_control_set(
      int unit,
      bcm_port_t port,
      bcm_port_control_t type,
      int value) override;
  int bcm_field_entry_destroy(int unit, bcm_field_entry_t /*entry*/) override;
  int bcm_field_group_destroy(int unit, bcm_field_group_t group) override;
  int bcm_cosq_init(int unit) override;
  int bcm_cosq_gport_sched_set(
      int unit,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq,
      int mode,
      int weight) override;
  int bcm_cosq_control_set(
      int unit,
      bcm_gport_t port,
      bcm_cos_queue_t cosq,
      bcm_cosq_control_t type,
      int arg) override;
  int bcm_cosq_port_profile_set(
      int unit,
      bcm_gport_t port,
      bcm_cosq_profile_type_t profile_type,
      int profile_id) override;
  int bcm_cosq_gport_bandwidth_set(
      int unit,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq,
      uint32 kbits_sec_min,
      uint32 kbits_sec_max,
      uint32 flags) override;
  int bcm_cosq_priority_group_mapping_profile_set(
      int unit,
      int profile_index,
      bcm_cosq_priority_group_mapping_profile_type_t type,
      int array_count,
      int* arg) override;
  int bcm_cosq_priority_group_pfc_priority_mapping_profile_set(
      int unit,
      int profile_index,
      int array_count,
      int* pg_array) override;
  int bcm_cosq_pfc_class_config_profile_set(
      int unit,
      int profile_index,
      int count,
      bcm_cosq_pfc_class_map_config_t* config_array) override;
  int bcm_cosq_pfc_deadlock_control_set(
      int unit,
      bcm_port_t port,
      int priority,
      bcm_cosq_pfc_deadlock_control_t type,
      int arg) override;
  int bcm_port_priority_group_config_set(
      int unit,
      bcm_gport_t gport,
      int priority_group,
      bcm_port_priority_group_config_t* prigrp_config) override;
  int bcm_cosq_port_priority_group_property_set(
      int unit,
      bcm_port_t gport,
      int priority_group_id,
      bcm_cosq_port_prigroup_control_t type,
      int arg) override;
  void bcm_field_hint_t_init(bcm_field_hint_t* /*hint*/) override {}
  int bcm_field_hints_create(int unit, bcm_field_hintid_t* hint_id) override;
  int bcm_field_hints_add(
      int unit,
      bcm_field_hintid_t hint_id,
      bcm_field_hint_t* hint) override;
  int bcm_field_hints_destroy(int unit, bcm_field_hintid_t hint_id) override;
  int bcm_field_hints_get(
      int unit,
      bcm_field_hintid_t hint_id,
      bcm_field_hint_t* hint) override;
  int bcm_field_init(int unit) override;
  int bcm_field_group_create_id(
      int unit,
      bcm_field_qset_t groupQset,
      bcm_field_group_t group,
      bcm_field_entry_t entry) override;
  int bcm_field_group_config_create(
      int unit,
      bcm_field_group_config_t* group_config) override;
  int bcm_field_entry_create(
      int unit,
      bcm_field_group_t group,
      bcm_field_entry_t* entry) override;
  int bcm_field_entry_install(int unit, bcm_field_entry_t entry) override;
  int bcm_field_action_add(
      int unit,
      bcm_field_entry_t entry,
      bcm_field_action_t action,
      uint32 param0,
      uint32 param1) override;
  int bcm_field_action_remove(
      int unit,
      bcm_field_entry_t entry,
      bcm_field_action_t action) override;
  int bcm_field_entry_prio_set(int unit, bcm_field_entry_t entry, int prio)
      override;
  int bcm_field_entry_enable_set(int unit, bcm_field_entry_t entry, int enable)
      override;
  int bcm_field_stat_create(
      int unit,
      bcm_field_group_t group,
      int nstat,
      bcm_field_stat_t* stat_arr,
      int* stat_id) override;
  int bcm_field_stat_destroy(int unit, int stat_id) override;
  int bcm_field_entry_stat_attach(
      int unit,
      bcm_field_entry_t entry,
      int stat_id) override;
  int bcm_field_entry_stat_detach(
      int unit,
      bcm_field_entry_t entry,
      int stat_id) override;
  int bcm_field_entry_reinstall(int unit, bcm_field_entry_t entry) override;
  int bcm_field_qualify_DstIp(
      int unit,
      bcm_field_entry_t entry,
      bcm_ip_t data,
      bcm_ip_t mask) override;
  int bcm_field_qualify_DstIp6(
      int unit,
      bcm_field_entry_t entry,
      bcm_ip6_t data,
      bcm_ip6_t mask) override;
  int bcm_field_qualify_SrcIp6(
      int unit,
      bcm_field_entry_t entry,
      bcm_ip6_t data,
      bcm_ip6_t mask) override;
  int bcm_field_qualify_L4DstPort(
      int unit,
      bcm_field_entry_t entry,
      bcm_l4_port_t data,
      bcm_l4_port_t mask) override;
  int bcm_field_qualify_L4SrcPort(
      int unit,
      bcm_field_entry_t entry,
      bcm_l4_port_t data,
      bcm_l4_port_t mask) override;
  int bcm_field_qualify_DstMac(
      int unit,
      bcm_field_entry_t entry,
      bcm_mac_t mac,
      bcm_mac_t macMask) override;
  int bcm_field_qualify_SrcMac(
      int unit,
      bcm_field_entry_t entry,
      bcm_mac_t mac,
      bcm_mac_t macMask) override;
  int bcm_mirror_init(int unit) override;
  int bcm_mirror_mode_set(int unit, int mode) override;
  int bcm_mirror_destination_create(
      int unit,
      bcm_mirror_destination_t* mirror_dest) override;
  int bcm_mirror_destination_destroy(int unit, bcm_gport_t mirror_dest_id)
      override;
  int bcm_mirror_port_dest_add(
      int unit,
      bcm_port_t port,
      uint32 flags,
      bcm_gport_t mirror_dest_id) override;
  int bcm_mirror_port_dest_delete(
      int unit,
      bcm_port_t port,
      uint32 flags,
      bcm_gport_t mirror_dest_id) override;
  int bcm_mirror_port_dest_delete_all(int unit, bcm_port_t port, uint32 flags)
      override;
  int bcm_l3_egress_create(
      int unit,
      uint32 flags,
      bcm_l3_egress_t* egr,
      bcm_if_t* if_id) override;
  int bcm_l3_ecmp_member_add(
      int unit,
      bcm_if_t ecmp_group_id,
      bcm_l3_ecmp_member_t* ecmp_member) override;
  int bcm_l3_egress_ecmp_add(
      int unit,
      bcm_l3_egress_ecmp_t* ecmp,
      bcm_if_t intf) override;
  int bcm_l3_ecmp_member_delete(
      int unit,
      bcm_if_t ecmp_group_id,
      bcm_l3_ecmp_member_t* ecmp_member) override;
  int bcm_l3_egress_ecmp_delete(
      int unit,
      bcm_l3_egress_ecmp_t* ecmp,
      bcm_if_t intf) override;
  int bcm_mpls_init(int /*unit*/) override;
  int bcm_mpls_tunnel_switch_add(int unit, bcm_mpls_tunnel_switch_t* info)
      override;
  int bcm_mpls_tunnel_switch_delete(int unit, bcm_mpls_tunnel_switch_t* info)
      override;
  int bcm_mpls_tunnel_initiator_set(
      int unit,
      bcm_if_t intf,
      int num_labels,
      bcm_mpls_egress_label_t* label_array) override;
  int bcm_mpls_tunnel_initiator_clear(int unit, bcm_if_t intf) override;
  int bcm_stat_custom_add(
      int unit,
      bcm_port_t port,
      bcm_stat_val_t type,
      bcm_custom_stat_trigger_t trigger) override;
  int bcm_port_phy_tx_set(int unit, bcm_port_t port, bcm_port_phy_tx_t* tx);
  int bcm_port_phy_control_set(
      int unit,
      bcm_port_t port,
      bcm_port_phy_control_t type,
      uint32 value) override;
  int bcm_l3_route_delete_by_interface(int unit, bcm_l3_route_t* l3_route)
      override;
  // BCM port resource APIs
  std::string cintForInitPortResource(std::string name);
  std::vector<std::string> cintForPortResource(
      std::string name,
      bcm_port_resource_t* resource);
  std::vector<std::string> cintForPortResources(
      int nport,
      bcm_port_resource_t* resource);
  int bcm_port_resource_speed_set(
      int unit,
      bcm_gport_t port,
      bcm_port_resource_t* resource) override;
  int bcm_port_resource_multi_set(
      int unit,
      int nport,
      bcm_port_resource_t* resource) override;
  int bcm_l3_route_delete(int unit, bcm_l3_route_t* l3_route) override;
  int bcm_l3_route_add(int unit, bcm_l3_route_t* l3_route) override;
  int bcm_l3_route_delete_all(int unit, bcm_l3_route_t* info) override;
  int bcm_l3_route_max_ecmp_set(int unit, int max) override;
  int bcm_l3_host_delete_by_interface(int unit, bcm_l3_host_t* l3_host)
      override;
  int bcm_l3_host_add(int unit, bcm_l3_host_t* l3_host) override;
  int bcm_l3_host_delete(int unit, bcm_l3_host_t* l3_host) override;
  int bcm_l3_host_delete_all(int unit, bcm_l3_host_t* info) override;
  int bcm_l3_intf_create(int unit, bcm_l3_intf_t* l3_intf) override;
  int bcm_l3_intf_delete(int unit, bcm_l3_intf_t* l3_intf) override;
  int bcm_port_vlan_member_set(int unit, bcm_port_t port, uint32 flags)
      override;
  int bcm_field_qualify_InPorts(
      int unit,
      bcm_field_entry_t entry,
      bcm_pbmp_t data,
      bcm_pbmp_t mask) override;
  int bcm_vlan_port_add(
      int unit,
      bcm_vlan_t vid,
      bcm_pbmp_t pbmp,
      bcm_pbmp_t ubmp) override;
  int bcm_vlan_create(int unit, bcm_vlan_t vid) override;
  int bcm_port_frame_max_set(int unit, bcm_port_t port, int size) override;
  int bcm_port_enable_set(int unit, bcm_port_t port, int enable) override;
  int bcm_l3_egress_multipath_create(
      int unit,
      uint32 flags,
      int intf_count,
      bcm_if_t* intf_array,
      bcm_if_t* mpintf) override;
  int bcm_l3_ecmp_create(
      int unit,
      uint32 options,
      bcm_l3_egress_ecmp_t* ecmp_info,
      int ecmp_member_count,
      bcm_l3_ecmp_member_t* ecmp_member_array) override;
  int bcm_l3_egress_ecmp_create(
      int unit,
      bcm_l3_egress_ecmp_t* ecmp,
      int intf_count,
      bcm_if_t* intf_array) override;
  int bcm_vlan_destroy_all(int unit) override;
  int bcm_port_untagged_vlan_set(int unit, bcm_port_t port, bcm_vlan_t vid)
      override;
  int bcm_vlan_destroy(int unit, bcm_vlan_t vid) override;
  int bcm_vlan_port_remove(int unit, bcm_vlan_t vid, bcm_pbmp_t pbmp) override;
  int bcm_l2_station_delete(int unit, int station_id) override;
  int bcm_tx(int unit, bcm_pkt_t* tx_pkt, void* cookie) override;
  int bcm_pktio_tx(int unit, bcm_pktio_pkt_t* tx_pkt) override;
#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_22))
  int bcm_pktio_txpmd_stat_attach(int unit, uint32 counter_id) override;

  int bcm_pktio_txpmd_stat_detach(int unit) override;
#endif
  int bcm_port_stat_enable_set(int unit, bcm_gport_t port, int enable) override;
  int bcm_port_stat_attach(int unit, bcm_port_t port, uint32 counterID_)
      override;
  int bcm_port_stat_detach_with_id(
      int unit,
      bcm_gport_t gPort,
      uint32 counterID) override;
  int bcm_stat_clear(int unit, bcm_port_t port) override;
#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_21))
  int bcm_port_fdr_config_set(
      int unit,
      bcm_port_t port,
      bcm_port_fdr_config_t* fdr_config) override;
#endif
  int bcm_port_speed_set(int unit, bcm_port_t port, int speed) override;
  int bcm_l3_egress_destroy(int unit, bcm_if_t intf) override;
  int bcm_l3_egress_multipath_add(int unit, bcm_if_t mpintf, bcm_if_t intf)
      override;
  int bcm_stg_create(int unit, bcm_stg_t* stg_ptr) override;
  int bcm_stg_default_set(int unit, bcm_stg_t stg) override;
  int bcm_stg_destroy(int unit, bcm_stg_t stg) override;
  int bcm_stg_stp_set(int unit, bcm_stg_t stg, bcm_port_t port, int stp_state)
      override;
  int bcm_vlan_default_set(int unit, bcm_vlan_t vid) override;
  int bcm_l3_ecmp_destroy(int unit, bcm_if_t ecmp_group_id) override;
  int bcm_l3_egress_ecmp_destroy(int unit, bcm_l3_egress_ecmp_t* ecmp) override;
  int bcm_stg_vlan_add(int unit, bcm_stg_t stg, bcm_vlan_t vid) override;
  int bcm_port_learn_set(int unit, bcm_port_t port, uint32 flags) override;
  int bcm_l2_station_add(int unit, int* station_id, bcm_l2_station_t* station)
      override;
  int bcm_pkt_free(int unit, bcm_pkt_t* pkt) override;
  int bcm_vlan_control_port_set(
      int unit,
      int port,
      bcm_vlan_control_port_t type,
      int arg) override;
  int bcm_pkt_alloc(int unit, int size, uint32 flags, bcm_pkt_t** pkt_buf)
      override;
  int bcm_cosq_mapping_set(int unit, bcm_cos_t priority, bcm_cos_queue_t cosq)
      override;
  int bcm_port_interface_set(int unit, bcm_port_t port, bcm_port_if_t intf)
      override;
  int bcm_l3_egress_multipath_destroy(int unit, bcm_if_t mpintf) override;
  int bcm_vlan_gport_delete_all(int unit, bcm_vlan_t vlan) override;
  int bcm_l3_egress_multipath_delete(int unit, bcm_if_t mpintf, bcm_if_t intf)
      override;
  int bcm_trunk_init(int unit) override;
  int bcm_trunk_set(
      int unit,
      bcm_trunk_t tid,
      bcm_trunk_info_t* t_data,
      int member_max,
      bcm_trunk_member_t* member_array) override;
  int bcm_trunk_destroy(int unit, bcm_trunk_t tid) override;
  int bcm_trunk_create(int unit, uint32 flags, bcm_trunk_t* tid) override;
  int bcm_trunk_member_add(
      int unit,
      bcm_trunk_t tid,
      bcm_trunk_member_t* member) override;
  int bcm_trunk_member_delete(
      int unit,
      bcm_trunk_t tid,
      bcm_trunk_member_t* member) override;
  int sh_process_command(int unit, char* cmd) override;
  int bcm_l2_addr_delete_by_port(
      int unit,
      bcm_module_t mod,
      bcm_port_t port,
      uint32 flags) override;
  int bcm_l3_ingress_create(
      int unit,
      bcm_l3_ingress_t* ing_intf,
      bcm_if_t* intf_id) override;
  int bcm_l3_ingress_destroy(int unit, bcm_if_t intf_id) override;
  int bcm_vlan_control_vlan_set(
      int unit,
      bcm_vlan_t vlan,
      bcm_vlan_control_vlan_t control) override;
  int bcm_port_phy_timesync_config_set(
      int unit,
      bcm_port_t port,
      bcm_port_phy_timesync_config_t* conf) override;
  int bcm_port_timesync_config_set(
      int unit,
      bcm_port_t port,
      int config_count,
      bcm_port_timesync_config_t* config_array) override;
  int bcm_time_interface_add(int unit, bcm_time_interface_t* intf) override;
  int bcm_time_interface_delete_all(int unit) override;
  int bcm_field_entry_flexctr_attach(
      int unit,
      bcm_field_entry_t entry,
      bcm_field_flexctr_config_t* flexctr_cfg) override;
  int bcm_field_entry_flexctr_detach(
      int unit,
      bcm_field_entry_t entry,
      bcm_field_flexctr_config_t* flexctr_cfg) override;
  int bcm_field_entry_remove(int unit, bcm_field_entry_t entry) override;
  int bcm_flexctr_action_create(
      int unit,
      int options,
      bcm_flexctr_action_t* action,
      uint32* stat_counter_id) override;
  int bcm_flexctr_action_destroy(int unit, uint32 stat_counter_id) override;

  int bcm_l3_route_stat_attach(
      int unit,
      bcm_l3_route_t* info,
      uint32 stat_counter_id) override;

  int bcm_l3_route_stat_detach(int unit, bcm_l3_route_t* info) override;

  int bcm_l3_route_flexctr_object_set(
      int unit,
      bcm_l3_route_t* info,
      uint32_t value) override;

  int bcm_stat_custom_group_create(
      int unit,
      uint32 mode_id,
      bcm_stat_object_t object,
      uint32* stat_counter_id,
      uint32* num_entries) override;

  int bcm_stat_group_destroy(int unit, uint32 stat_counter_id) override;

  int bcm_stat_group_create(
      int unit,
      bcm_stat_object_t object,
      bcm_stat_group_mode_t group_mode,
      uint32* stat_counter_id,
      uint32* num_entries) override;

  int bcm_l3_ingress_stat_attach(
      int unit,
      bcm_if_t intf_id,
      uint32 stat_counter_id) override;

  int bcm_l3_egress_stat_attach(
      int unit,
      bcm_if_t intf_id,
      uint32 stat_counter_id) override;

  int bcm_stat_group_mode_id_create(
      int unit,
      uint32 flags,
      uint32 total_counters,
      uint32 num_selectors,
      bcm_stat_group_mode_attr_selector_t* attr_selectors,
      uint32* mode_id) override;

  int bcm_stat_group_mode_id_destroy(int unit, uint32 mode_id) override;

  /*
   * Getters, traversal functions - Do nothing.
   * BcmCinter charter is to *program* and setup
   * ASIC state as a running wedge_agent did. APIs
   * used to query ASIC internals play no role in
   * setting this up
   */
  int bcm_switch_pkt_trace_info_get(
      int /*unit*/,
      uint32 /*options*/,
      uint8 /*port*/,
      int /*len*/,
      uint8* /*data*/,
      bcm_switch_pkt_trace_info_t* /*pkt_trace_info*/) override {
    return 0;
  }
  int bcm_field_range_get(
      int /*unit*/,
      bcm_field_range_t /*range*/,
      uint32* /*flags*/,
      bcm_l4_port_t* /*min*/,
      bcm_l4_port_t* /*max*/) override {
    return 0;
  }
  int bcm_port_phy_control_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_port_phy_control_t /*type*/,
      uint32* /*value*/) override {
    return 0;
  }
  int bcm_rx_active(int /*unit*/) override {
    return 0;
  }

  int bcm_rx_cosq_mapping_get(
      int /* unit */,
      int /* index */,
      bcm_rx_reasons_t* /* reasons */,
      bcm_rx_reasons_t* /* reasons_mask */,
      uint8* /* int_prio */,
      uint8* /* int_prio_mask */,
      uint32* /* packet_type */,
      uint32* /* packet_type_mask */,
      bcm_cos_queue_t* /* cosq */) override {
    return 0;
  }
  int bcm_rx_cosq_mapping_size_get(int /*unit*/, int* /*size*/) override {
    return 0;
  }
  int bcm_l2_traverse(
      int /*unit*/,
      bcm_l2_traverse_cb /*trav_fn*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_port_subsidiary_ports_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_pbmp_t* /*pbmp*/) override {
    return 0;
  }
  int bcm_field_group_enable_get(
      int /*unit*/,
      bcm_field_group_t /*group*/,
      int* /*enable*/) override {
    return 0;
  }
  int bcm_field_group_traverse(
      int /*unit*/,
      bcm_field_group_traverse_cb /*callback*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_l3_info(int /*unit*/, bcm_l3_info_t* /*l3info*/) override {
    return 0;
  }
  void bcm_l3_egress_ecmp_t_init(bcm_l3_egress_ecmp_t* /*ecmp*/) override {}
  int bcm_cosq_bst_profile_get(
      int /*unit*/,
      bcm_gport_t /*gport*/,
      bcm_cos_queue_t /*cosq*/,
      bcm_bst_stat_id_t /*bid*/,
      bcm_cosq_bst_profile_t* /*profile*/) override {
    return 0;
  }
  int bcm_switch_object_count_multi_get(
      int /*unit*/,
      int /*object_size*/,
      bcm_switch_object_t* /*object_array*/,
      int* /*entries*/) override {
    return 0;
  }
  int bcm_switch_object_count_get(
      int /*unit*/,
      bcm_switch_object_t /*object*/,
      int* /*entries*/) override {
    return 0;
  }
#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_19))
  int bcm_l3_alpm_resource_get(
      int /*unit*/,
      bcm_l3_route_group_t /*grp*/,
      bcm_l3_alpm_resource_t* /*resource*/) override {
    return 0;
  }
#endif
  int bcm_field_entry_multi_get(
      int /*unit*/,
      bcm_field_group_t /*group*/,
      int /*entry_size*/,
      bcm_field_entry_t* /*entry_array*/,
      int* /*entry_count*/) override {
    return 0;
  }
  int bcm_cosq_bst_stat_get(
      int /*unit*/,
      bcm_gport_t /*gport*/,
      bcm_cos_queue_t /*cosq*/,
      bcm_bst_stat_id_t /*bid*/,
      uint32 /*options*/,
      uint64* /*value*/) override {
    return 0;
  }
  int bcm_cosq_bst_stat_extended_get(
      int /*unit*/,
      bcm_cosq_object_id_t* /*id*/,
      bcm_bst_stat_id_t /*bid*/,
      uint32 /*options*/,
      uint64* /*value*/) override {
    return 0;
  }
  int bcm_field_entry_stat_get(
      int /*unit*/,
      bcm_field_entry_t /*entry*/,
      int* /*stat_id*/) override {
    return 0;
  }
  int bcm_l3_egress_ecmp_get(
      int /*unit*/,
      bcm_l3_egress_ecmp_t* /*ecmp*/,
      int /*intf_size*/,
      bcm_if_t* /*intf_array*/,
      int* /*intf_count*/) override {
    return 0;
  }

  int bcm_l3_enable_set(int unit, int enable) override;

  int bcm_rx_queue_max_get(int /*unit*/, bcm_cos_queue_t* /*cosq*/) override {
    return 0;
  }
  int bcm_field_group_get(
      int /*unit*/,
      bcm_field_group_t /*group*/,
      bcm_field_qset_t* /*qset*/) override {
    return 0;
  }
  int bcm_field_group_status_get(
      int /* unit */,
      bcm_field_group_t /* group */,
      bcm_field_group_status_t* /* status */) override {
    return 0;
  }
  int bcm_field_qualify_RangeCheck_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      int /* max_count */,
      bcm_field_range_t* /* range */,
      int* /* invert */,
      int* /* count */) override {
    return 0;
  }
  int bcm_field_entry_prio_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      int* /* prio */) override {
    return 0;
  }
  int bcm_field_stat_get(
      int /* unit */,
      int /* stat_id */,
      bcm_field_stat_t /* stat */,
      uint64* /* value */) override {
    return 0;
  }
  int bcm_field_stat_size(int /*unit*/, int /*stat_id*/, int* /*stat_size*/)
      override {
    return 0;
  }
  int bcm_field_stat_config_get(
      int /*unit*/,
      int /*stat_id*/,
      int /*nstat*/,
      bcm_field_stat_t* /*stat_arr*/) override {
    return 0;
  }
  int bcm_field_qualify_SrcIp6_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      bcm_ip6_t* /* data */,
      bcm_ip6_t* /* mask */) override {
    return 0;
  }
  int bcm_field_qualify_DstIp6_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      bcm_ip6_t* /* data */,
      bcm_ip6_t* /* mask */) override {
    return 0;
  }
  int bcm_field_qualify_L4SrcPort_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      bcm_l4_port_t* /* data */,
      bcm_l4_port_t* /* mask */) override {
    return 0;
  }
  int bcm_field_qualify_L4DstPort_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      bcm_l4_port_t* /* data */,
      bcm_l4_port_t* /* mask */) override {
    return 0;
  }
  int bcm_field_qualify_TcpControl_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      uint8* /* data */,
      uint8* /* mask */) override {
    return 0;
  }
  int bcm_field_qualify_SrcPort_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      bcm_module_t* /* data_modid */,
      bcm_module_t* /* mask_modid */,
      bcm_port_t* /* data_port */,
      bcm_port_t* /* mask_port */) override {
    return 0;
  }
  int bcm_field_qualify_DstPort_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      bcm_module_t* /* data_modid */,
      bcm_module_t* /* mask_modid */,
      bcm_port_t* /* data_port */,
      bcm_port_t* /* mask_port */) override {
    return 0;
  }
  int bcm_field_qualify_IpFrag_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      bcm_field_IpFrag_t* /* frag_info */) override {
    return 0;
  }
  int bcm_field_qualify_IpProtocol_get(
      int /*unit*/,
      bcm_field_entry_t /*entry*/,
      uint8* /*data*/,
      uint8* /*mask*/) override {
    return 0;
  }
  int bcm_field_qualify_DSCP_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      uint8* /* data */,
      uint8* /* mask */) override {
    return 0;
  }
  int bcm_field_qualify_IpType_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      bcm_field_IpType_t* /* type */) override {
    return 0;
  }
  int bcm_field_qualify_EtherType_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      uint16* /* data */,
      uint16* /* mask */) override {
    return 0;
  }
  int bcm_field_qualify_Ttl_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      uint8* /* data */,
      uint8* /* mask */) override {
    return 0;
  }
  int bcm_field_qualify_DstMac_get(
      int /*unit*/,
      bcm_field_entry_t /*entry*/,
      bcm_mac_t* /*data*/,
      bcm_mac_t* /*mask*/) override {
    return 0;
  }
  int bcm_field_qualify_SrcMac_get(
      int /*unit*/,
      bcm_field_entry_t /*entry*/,
      bcm_mac_t* /*data*/,
      bcm_mac_t* /*mask*/) override {
    return 0;
  }
  int bcm_field_entry_enable_get(
      int /* unit */,
      bcm_field_entry_t /* entry */,
      int* /* enable_flag */) override {
    return 0;
  }
  int bcm_field_qualify_DstClassL2_get(
      int /*unit*/,
      bcm_field_entry_t /*entry*/,
      uint32* /*data*/,
      uint32* /*mask*/) override {
    return 0;
  }
  int bcm_field_qualify_DstClassL3_get(
      int /*unit*/,
      bcm_field_entry_t /*entry*/,
      uint32* /*data*/,
      uint32* /*mask*/) override {
    return 0;
  }
  int bcm_field_qualify_PacketRes_get(
      int /*unit*/,
      bcm_field_entry_t /*entry*/,
      uint32* /*data*/,
      uint32* /*mask*/) override {
    return 0;
  }
  int bcm_field_qualify_OuterVlanId_get(
      int /*unit*/,
      bcm_field_entry_t /*entry*/,
      bcm_vlan_t* /*data*/,
      bcm_vlan_t* /*mask*/) override {
    return 0;
  }
  int bcm_cosq_control_get(
      int /* unit */,
      bcm_gport_t /* port */,
      bcm_cos_queue_t /* cosq */,
      bcm_cosq_control_t /* type */,
      int* /* arg */) override {
    return 0;
  }
  int bcm_cosq_port_profile_get(
      int /*unit*/,
      bcm_gport_t /*port*/,
      bcm_cosq_profile_type_t /*profile_type*/,
      int* /**profile_id*/) override {
    return 0;
  }
  int bcm_port_pause_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      int* /*pause_tx*/,
      int* /*pause_rx*/) override {
    return 0;
  }
  int bcm_port_sample_rate_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      int* /*ingress_rate*/,
      int* /*egress_rate*/) override {
    return 0;
  }
  int bcm_port_control_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_port_control_t /*type*/,
      int* /*value*/) override {
    return 0;
  }
  int bcm_field_action_get(
      int /*unit*/,
      bcm_field_entry_t /*entry*/,
      bcm_field_action_t /*action*/,
      uint32* /*param0*/,
      uint32* /*param1*/) override {
    return 0;
  }
  int bcm_info_get(int /*unit*/, bcm_info_t* /*info*/) override {
    return 0;
  }
  int bcm_port_loopback_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      uint32* /*value*/) override {
    return 0;
  }
  void bcm_cosq_gport_discard_t_init(
      bcm_cosq_gport_discard_t* /*discard*/) override {}
  int bcm_cosq_gport_discard_get(
      int /*unit*/,
      bcm_gport_t /*gport*/,
      bcm_cos_queue_t /*cosq*/,
      bcm_cosq_gport_discard_t* /*discard*/) override {
    return 0;
  }
  int bcm_cosq_gport_mapping_get(
      int /*unit*/,
      bcm_gport_t /*ing_port*/,
      bcm_cos_t /*priority*/,
      uint32_t /*flags*/,
      bcm_gport_t* /*gport*/,
      bcm_cos_queue_t* /*cosq*/) override {
    return 0;
  }
  int bcm_qos_map_multi_get(
      int /*unit*/,
      uint32 /*flags*/,
      int /*map_id*/,
      int /*array_size*/,
      bcm_qos_map_t* /*array*/,
      int* /*array_count*/) override {
    return 0;
  }
  int bcm_qos_multi_get(
      int /*unit*/,
      int /*array_size*/,
      int* /*map_ids_array*/,
      int* /*flags_array*/,
      int* /*array_count*/) override {
    return 0;
  }
  int bcm_qos_port_map_get(
      int /*unit*/,
      bcm_gport_t /*gport*/,
      int* /*ing_map*/,
      int* /*egr_map*/) override {
    return 0;
  }
  int bcm_qos_port_map_type_get(
      int /*unit*/,
      bcm_gport_t /*port*/,
      uint32 /*flags*/,
      int* /*map_id*/) override {
    return 0;
  }
  int bcm_port_dscp_map_mode_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      int* /*mode*/) override {
    return 0;
  }
  int bcm_switch_control_get(
      int /*unit*/,
      bcm_switch_control_t /*type*/,
      int* /*arg*/) override {
    return 0;
  }
  int bcm_cosq_bst_stat_sync(int /*unit*/, bcm_bst_stat_id_t /*bid*/) override {
    return 0;
  }
  int bcm_linkscan_update(int /*unit*/, bcm_pbmp_t /*pbmp*/) override {
    return 0;
  }
  int bcm_cosq_gport_sched_get(
      int /*unit*/,
      bcm_gport_t /*gport*/,
      bcm_cos_queue_t /*cosq*/,
      int* /*mode*/,
      int* /*weight*/) override {
    return 0;
  }
  int bcm_cosq_gport_bandwidth_get(
      int /*unit*/,
      bcm_gport_t /*gport*/,
      bcm_cos_queue_t /*cosq*/,
      uint32* /*kbits_sec_min*/,
      uint32* /*kbits_sec_max*/,
      uint32* /*flags*/) override {
    return 0;
  }
  int bcm_cosq_priority_group_mapping_profile_get(
      int /*unit*/,
      int /*profile_index*/,
      bcm_cosq_priority_group_mapping_profile_type_t /*type*/,
      int /*array_max*/,
      int* /*arg*/,
      int* /*array_count*/) override {
    return 0;
  }
  int bcm_cosq_priority_group_pfc_priority_mapping_profile_get(
      int /*unit*/,
      int /*profile_index*/,
      int /*array_max*/,
      int* /*pg_array*/,
      int* /*array_count*/) override {
    return 0;
  }
  int bcm_cosq_pfc_class_config_profile_get(
      int /*unit*/,
      int /*profile_index*/,
      int /*max_count*/,
      bcm_cosq_pfc_class_map_config_t* /*config_array*/,
      int* /*count*/) override {
    return 0;
  }
  int bcm_cosq_pfc_deadlock_control_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      int /*priority*/,
      bcm_cosq_pfc_deadlock_control_t /*type*/,
      int* /*arg*/) override {
    return 0;
  }
  int bcm_cosq_pfc_deadlock_recovery_event_register(
      int /*unit*/,
      bcm_cosq_pfc_deadlock_recovery_event_cb_t /*callback*/,
      void* /*userdata*/) override {
    return 0;
  }
  int bcm_cosq_pfc_deadlock_recovery_event_unregister(
      int /*unit*/,
      bcm_cosq_pfc_deadlock_recovery_event_cb_t /*callback*/,
      void* /*userdata*/) override {
    return 0;
  }
  int bcm_port_priority_group_config_get(
      int /*unit*/,
      bcm_gport_t /*gport*/,
      int /*priority_group*/,
      bcm_port_priority_group_config_t* /*prigrp_config*/) override {
    return 0;
  }
  int bcm_cosq_port_priority_group_property_get(
      int /*unit*/,
      bcm_port_t /*gport*/,
      int /*priority_group_id*/,
      bcm_cosq_port_prigroup_control_t /*type*/,
      int* /*arg*/) override {
    return 0;
  }
  int bcm_cosq_gport_traverse(
      int /*unit*/,
      bcm_cosq_gport_traverse_cb /*cb*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_trunk_bitmap_expand(int /*unit*/, bcm_pbmp_t* /*pbmp_ptr*/) override {
    return 0;
  }
  int bcm_mirror_destination_get(
      int /*unit*/,
      bcm_gport_t /*mirror_dest_id*/,
      bcm_mirror_destination_t* /*mirror_dest*/) override {
    return 0;
  }
  int bcm_mirror_port_dest_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      uint32 /*flags*/,
      int /*mirror_dest_size*/,
      bcm_gport_t* /*mirror_dest*/,
      int* /*mirror_dest_count*/) override {
    return 0;
  }
  int bcm_l3_egress_get(
      int /*unit*/,
      bcm_if_t /*intf*/,
      bcm_l3_egress_t* /*info*/) override {
    return 0;
  }
  int bcm_l3_egress_find(
      int /*unit*/,
      bcm_l3_egress_t* /*egr*/,
      bcm_if_t* /*intf*/) override {
    return 0;
  }
  int bcm_l3_egress_traverse(
      int /*unit*/,
      bcm_l3_egress_traverse_cb /*trav_fn*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_mirror_destination_traverse(
      int /*unit*/,
      bcm_mirror_destination_traverse_cb /*cb*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_mpls_tunnel_switch_get(
      int /*unit*/,
      bcm_mpls_tunnel_switch_t* /*info*/) override {
    return 0;
  }
  int bcm_mpls_tunnel_switch_traverse(
      int /*unit*/,
      bcm_mpls_tunnel_switch_traverse_cb /*cb*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_mpls_tunnel_initiator_get(
      int /*unit*/,
      bcm_if_t /*intf*/,
      int /*label_max*/,
      bcm_mpls_egress_label_t* /*label_array*/,
      int* /*label_count*/) override {
    return 0;
  }
  /**  Getters,  finders,  initers, traversers - skip since they don't affect*
   * ASIC state*/
  int bcm_port_gport_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_gport_t* /*gport*/) override {
    return 0;
  }
  int bcm_l3_intf_find_vlan(int /*unit*/, bcm_l3_intf_t* /*intf*/) override {
    return 0;
  }
  int bcm_l3_egress_ecmp_traverse(
      int /*unit*/,
      bcm_l3_egress_ecmp_traverse_cb /*trav_fn*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_l2_age_timer_get(int /*unit*/, int* /*age_seconds*/) override {
    return 0;
  }
  int bcm_l2_age_timer_set(int /*unit*/, int age_seconds) override;
  int bcm_vlan_list(int /*unit*/, bcm_vlan_data_t** /*listp*/, int* /*countp*/)
      override {
    return 0;
  }
  int bcm_switch_control_port_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_switch_control_t /*type*/,
      int* /*arg*/) override {
    return 0;
  }
  void bcm_port_config_t_init(bcm_port_config_t* /*config*/) override {}
  int bcm_port_config_get(int /*unit*/, bcm_port_config_t* /*config*/)
      override {
    return 0;
  }
  int bcm_l3_ecmp_get(
      int /*unit*/,
      bcm_l3_egress_ecmp_t* /*ecmp_info*/,
      int /*ecmp_member_size*/,
      bcm_l3_ecmp_member_t* /*ecmp_member_array*/,
      int* /*ecmp_member_count*/) override {
    return 0;
  }
  void bcm_knet_filter_t_init(bcm_knet_filter_t* /*filter*/) override {}
  int bcm_l3_host_find(int /*unit*/, bcm_l3_host_t* /*info*/) override {
    return 0;
  }
  int bcm_l3_egress_multipath_find(
      int /*unit*/,
      int /*intf_count*/,
      bcm_if_t* /*intf_array*/,
      bcm_if_t* /*mpintf*/) override {
    return 0;
  }
  int bcm_l3_route_traverse(
      int /*unit*/,
      uint32 /*flags*/,
      uint32 /*start*/,
      uint32 /*end*/,
      bcm_l3_route_traverse_cb /*trav_fn*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_linkscan_mode_get(int /*unit*/, bcm_port_t /*port*/, int* /*mode*/)
      override {
    return 0;
  }
  int bcm_port_local_get(
      int /*unit*/,
      bcm_gport_t /*gport*/,
      bcm_port_t* /*local_port*/) override {
    return 0;
  }
  int bcm_cosq_mapping_get(
      int /*unit*/,
      bcm_cos_t /*priority*/,
      bcm_cos_queue_t* /*cosq*/) override {
    return 0;
  }
  bcm_ip_t bcm_ip_mask_create(int /*len*/) override {
    return 0;
  }
  int bcm_l3_egress_multipath_get(
      int /*unit*/,
      bcm_if_t /*mpintf*/,
      int /*intf_size*/,
      bcm_if_t* /*intf_array*/,
      int* /*intf_count*/) override {
    return 0;
  }
  int bcm_stg_default_get(int /*unit*/, bcm_stg_t* /*stg_ptr*/) override {
    return 0;
  }
  int bcm_linkscan_enable_get(int /*unit*/, int* /*us*/) override {
    return 0;
  }
  int bcm_stg_stp_get(
      int /*unit*/,
      bcm_stg_t /*stg*/,
      bcm_port_t /*port*/,
      int* /*stp_state*/) override {
    return 0;
  }
  void bcm_l3_egress_t_init(bcm_l3_egress_t* /*egr*/) override {}
  int bcm_vlan_default_get(int /*unit*/, bcm_vlan_t* /*vid_ptr*/) override {
    return 0;
  }
  int bcm_l3_init(int /*unit*/) override {
    return 0;
  }
  int bcm_l2_station_get(
      int /*unit*/,
      int /*station_id*/,
      bcm_l2_station_t* /*station*/) override {
    return 0;
  }
  int bcm_port_ability_advert_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_port_ability_t* /*ability_mask*/) override {
    return 0;
  }
  int bcm_l3_route_multipath_get(
      int /*unit*/,
      bcm_l3_route_t* /*the_route*/,
      bcm_l3_route_t* /*path_array*/,
      int /*max_path*/,
      int* /*path_count*/) override {
    return 0;
  }
  int bcm_port_queued_count_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      uint32* /*count*/) override {
    return 0;
  }
  int bcm_port_learn_get(int /*unit*/, bcm_port_t /*port*/, uint32* /*flags*/)
      override {
    return 0;
  }
  int bcm_l3_route_get(int /*unit*/, bcm_l3_route_t* /*info*/) override {
    return 0;
  }
  int bcm_port_selective_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_port_info_t* /*info*/) override {
    return 0;
  }
  int bcm_stk_my_modid_get(int /*unit*/, int* /*my_modid*/) override {
    return 0;
  }
  int bcm_l3_intf_get(int /*unit*/, bcm_l3_intf_t* /*intf*/) override {
    return 0;
  }
  int bcm_l2_addr_get(
      int /*unit*/,
      bcm_mac_t /*mac_addr*/,
      bcm_vlan_t /*vid*/,
      bcm_l2_addr_t* /*l2addr*/) override {
    return 0;
  }
  int bcm_stat_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_stat_val_t /*type*/,
      uint64* /*value*/) override {
    return 0;
  }
  int bcm_stat_sync_multi_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      int /*nstat*/,
      bcm_stat_val_t* /*stat_arr*/,
      uint64* /*value_arr*/) override {
    return 0;
  }
  int bcm_l3_intf_find(int /*unit*/, bcm_l3_intf_t* /*intf*/) override {
    return 0;
  }
  int bcm_stg_list(int /*unit*/, bcm_stg_t** /*list*/, int* /*count*/)
      override {
    return 0;
  }
  int bcm_port_vlan_member_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      uint32* /*flags*/) override {
    return 0;
  }
  int bcm_pkt_flags_init(
      int /*unit*/,
      bcm_pkt_t* /*pkt*/,
      uint32 /*init_flags*/) override {
    return 0;
  }
  int bcm_l3_egress_multipath_traverse(
      int /*unit*/,
      bcm_l3_egress_multipath_traverse_cb /*trav_fn*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_l3_host_traverse(
      int /*unit*/,
      uint32 /*flags*/,
      uint32 /*start*/,
      uint32 /*end*/,
      bcm_l3_host_traverse_cb /*cb*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_cosq_bst_stat_multi_get(
      int /*unit*/,
      bcm_gport_t /*gport*/,
      bcm_cos_queue_t /*cosq*/,
      uint32 /*options*/,
      int /*max_values*/,
      bcm_bst_stat_id_t* /*id_list*/,
      uint64* /*values*/) override {
    return 0;
  }
  int bcm_l3_egress_ecmp_find(
      int /*unit*/,
      int /*intf_count*/,
      bcm_if_t* /*intf_array*/,
      bcm_l3_egress_ecmp_t* /*ecmp*/) override {
    return 0;
  }
  int bcm_port_dtag_mode_get(int /*unit*/, bcm_port_t /*port*/, int* /*mode*/)
      override {
    return 0;
  }
  int bcm_knet_filter_traverse(
      int /*unit*/,
      bcm_knet_filter_traverse_cb /*trav_fn*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_port_speed_max(int /*unit*/, bcm_port_t /*port*/, int* /*speed*/)
      override {
    return 0;
  }
  int bcm_port_ability_local_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_port_ability_t* /*local_ability_mask*/) override {
    return 0;
  }
  int bcm_port_enable_get(int /*unit*/, bcm_port_t /*port*/, int* /*enable*/)
      override {
    return 0;
  }
  int bcm_port_frame_max_get(int /*unit*/, bcm_port_t /*port*/, int* /*size*/)
      override {
    return 0;
  }
  int bcm_l3_route_max_ecmp_get(int /*unit*/, int* /*max*/) override {
    return 0;
  }
  int bcm_port_interface_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_port_if_t* /*intf*/) override {
    return 0;
  }
  int bcm_port_speed_get(int /*unit*/, bcm_port_t /*port*/, int* /*speed*/)
      override {
    return 0;
  }
  int bcm_knet_netif_traverse(
      int /*unit*/,
      bcm_knet_netif_traverse_cb /*trav_fn*/,
      void* /*user_data*/) override {
    return 0;
  }
  int bcm_linkscan_mode_set(int /*unit*/, bcm_port_t /*port*/, int /*mode*/)
      override;
  int bcm_vlan_list_destroy(
      int /*unit*/,
      bcm_vlan_data_t* /*list*/,
      int /*count*/) override {
    return 0;
  }
  int bcm_attach_max(int* /*max_units*/) override {
    return 0;
  }
  int bcm_linkscan_detach(int /*unit*/) override {
    return 0;
  }
  int bcm_rx_cfg_get(int /*unit*/, bcm_rx_cfg_t* /*cfg*/) override {
    return 0;
  }
  int bcm_switch_event_unregister(
      int /*unit*/,
      bcm_switch_event_cb_t /*cb*/,
      void* /*userdata*/) override {
    return 0;
  }
  int bcm_attach(
      int /*unit*/,
      char* /*type*/,
      char* /*subtype*/,
      int /*remunit*/) override {
    return 0;
  }
  int bcm_l2_addr_delete(int /*unit*/, bcm_mac_t /*mac*/, bcm_vlan_t /*vid*/)
      override {
    return 0;
  }
  int bcm_port_link_status_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      int* /*status*/) override {
    return 0;
  }
  char* bcm_port_name(int /*unit*/, int /*port*/) override {
    return nullptr;
  }
  int bcm_linkscan_enable_set(int /*unit*/, int /*us*/) override;
  int bcm_knet_init(int /*unit*/) override {
    return 0;
  }
  int bcm_stat_multi_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      int /*nstat*/,
      bcm_stat_val_t* /*stat_arr*/,
      uint64* /*value_arr*/) override {
    return 0;
  }
  int bcm_detach(int /*unit*/) override {
    return 0;
  }
  int _bcm_shutdown(int /*unit*/) override {
    return 0;
  }
  int soc_shutdown(int /*unit*/) override {
    return 0;
  }
  int bcm_port_untagged_vlan_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_vlan_t* /*vid_ptr*/) override {
    return 0;
  }
  int bcm_linkscan_unregister(int /*unit*/, bcm_linkscan_handler_t /*f*/)
      override {
    return 0;
  }
  int bcm_ip6_mask_create(bcm_ip6_t /*ip6*/, int /*len*/) override {
    return 0;
  }
  void bcm_knet_netif_t_init(bcm_knet_netif_t* /*netif*/) override {}
  int bcm_rx_control_get(int /*unit*/, bcm_rx_control_t /*type*/, int* /*arg*/)
      override {
    return 0;
  }
  int bcm_l2_addr_add(int /*unit*/, bcm_l2_addr_t* /*l2addr*/) override {
    return 0;
  }
  int bcm_attach_check(int /*unit*/) override {
    return 0;
  }
  int bcm_linkscan_mode_set_pbm(int /*unit*/, bcm_pbmp_t /*pbm*/, int /*mode*/)
      override {
    return 0;
  }
  int bcm_switch_event_register(
      int /*unit*/,
      bcm_switch_event_cb_t /*cb*/,
      void* /*userdata*/) override {
    return 0;
  }
  int bcm_linkscan_register(int /*unit*/, bcm_linkscan_handler_t /*f*/)
      override {
    return 0;
  }
  int bcm_trunk_get(
      int /*unit*/,
      bcm_trunk_t /*tid*/,
      bcm_trunk_info_t* /*t_data*/,
      int /*member_max*/,
      bcm_trunk_member_t* /*member_array*/,
      int* /*member_count*/) override {
    return 0;
  }
  int bcm_trunk_find(
      int /* unit */,
      bcm_module_t /* module */,
      bcm_port_t /* port */,
      bcm_trunk_t* /* tid */) override {
    return 0;
  }
  // Since QCM is not supported after 6.5.16, we no longer need to log these
  // functions.
  void bcm_collector_export_profile_t_init(
      bcm_collector_export_profile_t* /* export_profile_info */) override {}
  void bcm_collector_info_t_init(
      bcm_collector_info_t* /* collector_info */) override {}
  int bcm_collector_create(
      int /*unit*/,
      uint32 /*options*/,
      bcm_collector_t* /*collector_id*/,
      bcm_collector_info_t* /*collector_info*/) override {
    return 0;
  }
  int bcm_collector_destroy(int /*unit*/, bcm_collector_t /*id*/) override {
    return 0;
  }
  int bcm_collector_export_profile_create(
      int /*unit*/,
      uint32 /*options*/,
      int* /*export_profile_id*/,
      bcm_collector_export_profile_t* /*export_profile_info*/) override {
    return 0;
  }
  int bcm_collector_export_profile_destroy(
      int /*unit*/,
      int /*export_profile_id*/) override {
    return 0;
  }
  void bcm_port_phy_timesync_config_t_init(
      bcm_port_phy_timesync_config_t* /*conf*/) override {}
  void bcm_port_timesync_config_t_init(
      bcm_port_timesync_config_t* /*port_timesync_config*/) override {}
  void bcm_time_interface_t_init(bcm_time_interface_t* /*intf*/) override {}
  // In general we don't need to log get calls because
  // 1. they introduce logging overhead,
  // 2. we never inspect the returned value, and
  // 3. they don't change the ASIC state.
  // Hence removing the following get calls that are previously logged.
  int bcm_port_phy_tx_get(
      int /*unit*/,
      bcm_port_t /*port*/,
      bcm_port_phy_tx_t* /*tx*/) {
    return 0;
  }
  int bcm_port_resource_speed_get(
      int /*unit*/,
      bcm_gport_t /*port*/,
      bcm_port_resource_t* /* resource */) override {
    return 0;
  }
  int bcm_vlan_control_vlan_get(
      int /*unit*/,
      bcm_vlan_t /*vlan*/,
      bcm_vlan_control_vlan_t* /*control*/) override {
    return 0;
  }

#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_21))
  int bcm_port_fdr_config_get(
      int unit,
      bcm_port_t port,
      bcm_port_fdr_config_t* fdr_config) override {
    return 0;
  }

  int bcm_port_fdr_stats_get(
      int unit,
      bcm_port_t port,
      bcm_port_fdr_stats_t* fdr_stats) override {
    return 0;
  }
#endif

  int bcm_port_ifg_get(
      int /* unit */,
      bcm_port_t /* port */,
      int /* speed */,
      bcm_port_duplex_t /* duplex */,
      int* /* bit_times */) override {
    return 0;
  }

  int bcm_port_ifg_set(
      int unit,
      bcm_port_t port,
      int speed,
      bcm_port_duplex_t duplex,
      int bit_times) override;

 private:
  enum class Dir { SRC, DST };
  int bcmFieldQualifyIp6(
      int unit,
      bcm_field_entry_t entry,
      bcm_ip6_t data,
      bcm_ip6_t mask,
      Dir dir);

  int bcmFieldQualifyL4Port(
      int unit,
      bcm_field_entry_t entry,
      bcm_l4_port_t data,
      bcm_l4_port_t mask,
      Dir dir);

  int bcmFieldQualifyMac(
      int unit,
      bcm_field_entry_t entry,
      bcm_mac_t mac,
      bcm_mac_t macMask,
      Dir dir);

  /*
   * Declare global variable for use in generated cint
   */
  void setupGlobals();
  /*
   * We use tmp variables for use in cint calls. Following
   * getNextTmpXXXVar return the name of next variable for use
   * in cint generation.
   */
  std::string getNextTmpV6Var();
  std::string getNextTmpMacVar();
  std::string getNextTmpStatArray();
  std::string getNextTmpTrunkMemberArray();
  std::string getNextTmpPortBitmapVar();
  std::string getNextTmpReasonsVar();
  std::string getNextFieldEntryVar();
  std::string getNextQosMapVar();
  std::string getNextL3IntfIdVar();
  std::string getNextStatVar();
  std::string getNextMirrorDestIdVar();
  std::string getNextRangeVar();
  std::string getNextStgVar();
  std::string getNextPriorityToPgVar();
  std::string getNextPfcPriToPgVar();
  std::string getNextPfcPriToQueueVar();
  std::string getNextPgConfigVar();
  std::string getNextPptConfigVar();
  std::string getNextPtConfigVar();
  std::string getNextPtConfigArrayVar();
  std::string getNextTimeInterfaceVar();
  std::string getNextTimeSpecVar();
  std::string getNextStateCounterVar();
  std::string getNextEthertypeVar();

  /*
   * Wrap a generated cint function call with return error code
   * check
   */
  std::vector<std::string> wrapFunc(const std::string& funcCall) const;
  /*
   * Write cint lines to file
   */
  void writeCintLine(const std::string& cint);
  template <typename C>
  void writeCintLines(C&& lines);
  std::vector<std::string> cintForQset(const bcm_field_qset_t& qset) const;
  std::vector<std::string> cintForAset(const bcm_field_aset_t& aset) const;
  std::vector<std::string> cintForFpGroupConfig(
      const bcm_field_group_config_t* group_config) const;
  /*
   * Create a ipv6 variable initialized from in
   * Return cint for initialization and the variable name
   */
  std::pair<std::string, std::string> cintForIp6(const bcm_ip6_t in);
  /*
   * Create a mac variable initialized from in
   * Return cint for initialization and the variable name
   */
  std::pair<std::string, std::string> cintForMac(const bcm_mac_t in);
  /*
   * Create a field stats array
   * Return cint for initialization and the variable name
   */
  std::pair<std::string, std::string> cintForStatArray(
      const bcm_field_stat_t* statArray,
      int statSize);
  std::vector<std::string> cintForQosMapEntry(
      uint32 flags,
      const bcm_qos_map_t* qosMapEntry) const;

  std::vector<std::string> cintForCosQDiscard(
      const bcm_cosq_gport_discard_t& discard) const;

  std::vector<std::string> cintForL3Ecmp(const bcm_l3_egress_ecmp_t& ecmp);

  std::vector<std::string> cintForL3Route(const bcm_l3_route_t& l3_route);

  std::vector<std::string> cintForL3Host(const bcm_l3_host_t& l3_host);

  std::vector<std::string> cintForL3Intf(const bcm_l3_intf_t& l3_intf);

  std::vector<std::string> cintForMirrorDestination(
      const bcm_mirror_destination_t* mirror_dest);

  std::vector<std::string> cintForBcmUdfHashConfig(
      const bcm_udf_hash_config_t& config);

  std::vector<std::string> cintForBcmUdfAllocHints(
      const bcm_udf_alloc_hints_t* hints);

  std::vector<std::string> cintForBcmUdfInfo(const bcm_udf_t* udf_info);

  std::vector<std::string> cintForBcmUdfPktFormatInfo(
      const bcm_udf_pkt_format_info_t& pktFormat);

  std::vector<std::string> cintForTrunkInfo(const bcm_trunk_info_t& t_data);

  std::vector<std::string> cintForTrunkMember(const bcm_trunk_member_t& member);

  std::vector<std::string> cintForTrunkId(const bcm_trunk_t& tid);

  std::pair<std::string, std::vector<std::string>> cintForTrunkMemberArray(
      const bcm_trunk_member_t* trunkMemberArray,
      int member_max);

  std::vector<std::string> cintForL3Egress(const bcm_l3_egress_t* l3_egress);

  std::vector<std::string> cintForEgressId(const bcm_if_t& if_id);

  std::vector<std::string> cintForLabelsArray(
      int num_labels,
      bcm_mpls_egress_label_t* labels);

  std::vector<std::string> cintForMplsTunnelSwitch(
      const bcm_mpls_tunnel_switch_t& switch_info);

  std::vector<std::string> cintForEcmpMembersArray(
      const bcm_l3_ecmp_member_t* members,
      int member_count);

  std::vector<std::string> cintForPathsArray(
      const bcm_if_t* paths,
      int path_count);

  std::pair<std::string, std::vector<std::string>> cintForPortBitmap(
      bcm_pbmp_t pbmp);

  std::vector<std::string> cintForUntaggedPortBitmap(bcm_pbmp_t ubmp);

  std::pair<std::string, std::vector<std::string>> cintForReasons(
      bcm_rx_reasons_t reasons);

  std::vector<std::string> cintForL2Station(bcm_l2_station_t* station);

  std::vector<std::string> cintForCosqBstProfile(
      bcm_cosq_bst_profile_t* profile);

  std::vector<std::string> cintForL3Ingress(const bcm_l3_ingress_t* l3_ingress);

  std::vector<std::string> cintForPortPhyTimesyncConfig(
      bcm_port_phy_timesync_config_t* conf,
      const std::string& pptConfigVar);

  std::vector<std::string> cintForPortTimesyncConfig(
      const bcm_port_timesync_config_t& conf,
      const std::string& ppConfigVar);

  std::vector<std::string> cintForPortTimesyncConfigArray(
      int config_count,
      bcm_port_timesync_config_t* config_array,
      const std::string& ppConfigArrayVar);

  std::vector<std::string> cintForTimeSpec(
      const bcm_time_spec_s& timeSpec,
      const std::string& timeSpecVar);

  std::vector<std::string> cintForTimeInterface(
      bcm_time_interface_t* intf,
      const std::string& timeInterfaceVar);

  std::vector<std::string> cintForFlexctrConfig(
      bcm_field_flexctr_config_t* flexctr_cfg);

  std::vector<std::string> cintForFlexCtrValueOperation(
      bcm_flexctr_value_operation_t* operation,
      const std::string& varname);

  std::vector<std::string> cintForFlexCtrAction(
      bcm_flexctr_action_t* flexctr_action);

  std::vector<std::string> cintForFlexCtrTrigger(
      bcm_flexctr_trigger_t& flexctr_trigger);

  std::vector<std::string> cintForHint(bcm_field_hint_t hint);

#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_21))
  std::vector<std::string> cintForPortFdrConfig(
      bcm_port_fdr_config_t fdr_config);
#endif

  /*
   * Synchronize access to data structures for access from multiple
   * threads.
   */
  std::atomic<uint> tmpV6Created_{0};
  std::atomic<uint> tmpMacCreated_{0};
  std::atomic<uint> linesWritten_{0};
  std::atomic<uint> tmpStatArrayCreated_{0};
  std::atomic<uint> tmpTrunkMemberArrayCreated_{0};
  std::atomic<uint> tmpPortBitmapCreated_{0};
  std::atomic<uint> tmpReasonsCreated_{0};
  std::atomic<uint> fieldEntryCreated_{0};
  std::atomic<uint> qosMapCreated_{0};
  std::atomic<uint> l3IntfCreated_{0};
  std::atomic<uint> statCreated_{0};
  std::atomic<uint> mirrorDestIdCreated_{0};
  std::atomic<uint> rangeCreated_{0};
  std::atomic<uint> stgCreated_{0};
  std::atomic<uint> tmpProrityToPgCreated_{0};
  std::atomic<uint> tmpPfcPriToPgCreated_{0};
  std::atomic<uint> tmpPfcPriToQueueCreated_{0};
  std::atomic<uint> tmpPgConfigCreated_{0};
  std::atomic<uint> tmpPptConfigCreated_{0};
  std::atomic<uint> tmpPtConfigCreated_{0};
  std::atomic<uint> tmpPtConfigArrayCreated_{0};
  std::atomic<uint> tmpTimeInterfaceCreated_{0};
  std::atomic<uint> tmpTimeSpecCreated_{0};
  std::atomic<uint> tmpStateCounterCreated_{0};
  std::atomic<uint> tmpEthertype_{0};

  std::unique_ptr<AsyncLogger> asyncLogger_;

  /*
   * Mapping from the resource handles currently held by wedge_agent to
   * their corresponding variables in the cint script.
   */
  folly::Synchronized<std::map<bcm_field_entry_t, std::string>> fieldEntryVars;
  folly::Synchronized<std::map<int, std::string>> qosMapIdVars;
  folly::Synchronized<std::map<bcm_if_t, std::string>> l3IntfIdVars;
  folly::Synchronized<std::map<int, std::string>> statIdVars;
  folly::Synchronized<std::map<int, std::string>> l2StationIdVars;
  folly::Synchronized<std::map<bcm_gport_t, std::string>> mirrorDestIdVars;
  folly::Synchronized<std::map<bcm_field_range_t, std::string>> rangeIdVars;
  folly::Synchronized<std::map<bcm_stg_t, std::string>> stgIdVars;
  folly::Synchronized<std::map<uint32, std::string>> statCounterIdVars;
};

} // namespace facebook::fboss
