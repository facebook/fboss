#pragma once

extern "C" {
#include <opennsl/cosq.h>
#include <opennsl/error.h>
#include <opennsl/init.h>
#include <opennsl/knet.h>
#include <opennsl/l2.h>
#include <opennsl/l3.h>
#include <opennsl/link.h>
#include <opennsl/mpls.h>
#include <opennsl/pkt.h>
#include <opennsl/port.h>
#include <opennsl/rx.h>
#include <opennsl/stack.h>
#include <opennsl/stat.h>
#include <opennsl/stg.h>
#include <opennsl/switch.h>
#include <opennsl/trunk.h>
#include <opennsl/tx.h>
#include <opennsl/types.h>
#include <opennsl/vlan.h>
}

#include <memory>

#include <gmock/gmock.h>

namespace facebook { namespace fboss {

class OpenNSLInterface {
 public:
  virtual ~OpenNSLInterface() = default;

  virtual int opennsl_port_gport_get(
      int unit,
      opennsl_port_t port,
      opennsl_gport_t* gport) = 0;

  virtual int
  opennsl_port_vlan_member_set(int unit, opennsl_port_t port, uint32 flags) = 0;

  virtual int opennsl_l2_age_timer_set(int unit, int age_seconds) = 0;

  virtual int opennsl_stg_destroy(int unit, opennsl_stg_t stg) = 0;

  virtual int opennsl_l3_intf_find_vlan(int unit, opennsl_l3_intf_t* intf) = 0;

  virtual int opennsl_knet_netif_create(
      int unit,
      opennsl_knet_netif_t* netif) = 0;

  virtual void opennsl_trunk_info_t_init(opennsl_trunk_info_t* trunk_info) = 0;

  virtual int opennsl_trunk_init(int unit) = 0;

  virtual int opennsl_trunk_find(
      int unit,
      opennsl_module_t modid,
      opennsl_port_t port,
      opennsl_trunk_t* tid) = 0;

  virtual int
  opennsl_trunk_create(int unit, uint32 flags, opennsl_trunk_t* tid) = 0;

  virtual int opennsl_trunk_detach(int unit) = 0;

  virtual void opennsl_trunk_member_t_init(
      opennsl_trunk_member_t* trunk_member) = 0;

  virtual int opennsl_trunk_member_delete(
      int unit,
      opennsl_trunk_t tid,
      opennsl_trunk_member_t* member) = 0;

  virtual int opennsl_trunk_get(
      int unit,
      opennsl_trunk_t tid,
      opennsl_trunk_info_t* t_data,
      int member_max,
      opennsl_trunk_member_t* member_array,
      int* member_count) = 0;

  virtual int opennsl_trunk_set(
      int unit,
      opennsl_trunk_t tid,
      opennsl_trunk_info_t* trunk_info,
      int member_count,
      opennsl_trunk_member_t* member_array) = 0;

  virtual int opennsl_trunk_psc_set(int unit, opennsl_trunk_t tid, int psc) = 0;

  virtual int opennsl_trunk_member_add(
      int unit,
      opennsl_trunk_t tid,
      opennsl_trunk_member_t* member) = 0;

  virtual int opennsl_trunk_chip_info_get(
      int unit,
      opennsl_trunk_chip_info_t* ta_info) = 0;

  virtual int opennsl_trunk_destroy(int unit, opennsl_trunk_t tid) = 0;

  virtual int opennsl_l2_traverse(
      int unit,
      opennsl_l2_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int opennsl_l3_route_delete_by_interface(
      int unit,
      opennsl_l3_route_t* info) = 0;

  virtual int opennsl_l3_host_delete_by_interface(
      int unit,
      opennsl_l3_host_t* info) = 0;

  virtual int opennsl_l3_egress_ecmp_traverse(
      int unit,
      opennsl_l3_egress_ecmp_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int opennsl_l2_age_timer_get(int unit, int* age_seconds) = 0;

  virtual int
  opennsl_vlan_list(int unit, opennsl_vlan_data_t** listp, int* countp) = 0;

  virtual int opennsl_vlan_destroy_all(int unit) = 0;

  virtual int opennsl_l3_egress_ecmp_add(
      int unit,
      opennsl_l3_egress_ecmp_t* ecmp,
      opennsl_if_t intf) = 0;

  virtual int
  opennsl_rx_unregister(int unit, opennsl_rx_cb_f callback, uint8 priority) = 0;

  virtual void opennsl_l3_route_t_init(opennsl_l3_route_t* route) = 0;

  virtual int opennsl_switch_control_port_get(
      int unit,
      opennsl_port_t port,
      opennsl_switch_control_t type,
      int* arg) = 0;

  virtual int opennsl_info_get(int unit, opennsl_info_t* info) = 0;

  virtual int opennsl_port_untagged_vlan_set(
      int unit,
      opennsl_port_t port,
      opennsl_vlan_t vid) = 0;

  virtual int
  opennsl_rx_control_set(int unit, opennsl_rx_control_t type, int arg) = 0;

  virtual int opennsl_knet_netif_destroy(int unit, int netif_id) = 0;

  virtual int opennsl_port_config_get(
      int unit,
      opennsl_port_config_t* config) = 0;

  virtual int opennsl_vlan_destroy(int unit, opennsl_vlan_t vid) = 0;

  virtual int opennsl_stg_create(int unit, opennsl_stg_t* stg_ptr) = 0;

  virtual int opennsl_l3_ecmp_get(
      int unit,
      opennsl_l3_egress_ecmp_t* ecmp_info,
      int ecmp_member_size,
      opennsl_l3_ecmp_member_t* ecmp_member_array,
      int* ecmp_member_count) = 0;

  virtual int
  opennsl_linkscan_mode_set(int unit, opennsl_port_t port, int mode) = 0;

  virtual int opennsl_l3_host_delete(int unit, opennsl_l3_host_t* ip_addr) = 0;

  virtual int opennsl_l3_intf_create(int unit, opennsl_l3_intf_t* intf) = 0;

  virtual int opennsl_l3_route_delete(int unit, opennsl_l3_route_t* info) = 0;

  virtual int opennsl_cosq_bst_stat_sync(
      int unit,
      opennsl_bst_stat_id_t bid) = 0;

  virtual void opennsl_knet_filter_t_init(opennsl_knet_filter_t* filter) = 0;

  virtual int opennsl_l3_host_find(int unit, opennsl_l3_host_t* info) = 0;

  virtual int opennsl_stg_default_set(int unit, opennsl_stg_t stg) = 0;

  virtual int opennsl_port_control_get(
      int unit,
      opennsl_port_t port,
      opennsl_port_control_t type,
      int* value) = 0;

  virtual int opennsl_l3_egress_multipath_find(
      int unit,
      int intf_count,
      opennsl_if_t* intf_array,
      opennsl_if_t* mpintf) = 0;

  virtual int opennsl_switch_control_port_set(
      int unit,
      opennsl_port_t port,
      opennsl_switch_control_t type,
      int arg) = 0;

  virtual int
  opennsl_vlan_list_destroy(int unit, opennsl_vlan_data_t* list, int count) = 0;

  virtual int opennsl_vlan_port_remove(
      int unit,
      opennsl_vlan_t vid,
      opennsl_pbmp_t pbmp) = 0;

  virtual int opennsl_l3_route_traverse(
      int unit,
      uint32 flags,
      uint32 start,
      uint32 end,
      opennsl_l3_route_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int opennsl_l2_station_delete(int unit, int station_id) = 0;

  virtual int
  opennsl_linkscan_mode_get(int unit, opennsl_port_t port, int* mode) = 0;

  virtual int opennsl_switch_pkt_trace_info_get(
      int unit,
      uint32 options,
      uint8 port,
      int len,
      uint8* data,
      opennsl_switch_pkt_trace_info_t* pkt_trace_info) = 0;

  virtual opennsl_ip_t opennsl_ip_mask_create(int len) = 0;

  virtual int opennsl_l3_egress_multipath_get(
      int unit,
      opennsl_if_t mpintf,
      int intf_size,
      opennsl_if_t* intf_array,
      int* intf_count) = 0;

  virtual int opennsl_stg_default_get(int unit, opennsl_stg_t* stg_ptr) = 0;

  virtual int opennsl_linkscan_enable_get(int unit, int* us) = 0;

  virtual int opennsl_stg_stp_get(
      int unit,
      opennsl_stg_t stg,
      opennsl_port_t port,
      int* stp_state) = 0;

  virtual int opennsl_attach_max(int* max_units) = 0;

  virtual int opennsl_port_local_get(
      int unit,
      opennsl_gport_t gport,
      opennsl_port_t* local_port) = 0;

  virtual void opennsl_l3_egress_ecmp_t_init(
      opennsl_l3_egress_ecmp_t* ecmp) = 0;

  virtual int opennsl_cosq_bst_profile_set(
      int unit,
      opennsl_gport_t gport,
      opennsl_cos_queue_t cosq,
      opennsl_bst_stat_id_t bid,
      opennsl_cosq_bst_profile_t* profile) = 0;

  virtual int opennsl_cosq_mapping_get(
      int unit,
      opennsl_cos_t priority,
      opennsl_cos_queue_t* cosq) = 0;

  virtual int opennsl_port_control_set(
      int unit,
      opennsl_port_t port,
      opennsl_port_control_t type,
      int value) = 0;

  virtual void opennsl_l3_info_t_init(opennsl_l3_info_t* info) = 0;

  virtual int opennsl_tx(int unit, opennsl_pkt_t* tx_pkt, void* cookie) = 0;

  virtual int opennsl_l3_egress_get(
      int unit,
      opennsl_if_t intf,
      opennsl_l3_egress_t* egr) = 0;

  virtual void opennsl_l3_egress_t_init(opennsl_l3_egress_t* egr) = 0;

  virtual int
  opennsl_port_stat_enable_set(int unit, opennsl_gport_t port, int enable) = 0;

  virtual int opennsl_vlan_default_get(int unit, opennsl_vlan_t* vid_ptr) = 0;

  virtual int opennsl_l3_init(int unit) = 0;

  virtual int opennsl_l2_station_get(
      int unit,
      int station_id,
      opennsl_l2_station_t* station) = 0;

  virtual int opennsl_linkscan_detach(int unit) = 0;

  virtual int opennsl_rx_stop(int unit, opennsl_rx_cfg_t* cfg) = 0;

  virtual int opennsl_l3_egress_multipath_create(
      int unit,
      uint32 flags,
      int intf_count,
      opennsl_if_t* intf_array,
      opennsl_if_t* mpintf) = 0;

  virtual int opennsl_port_ability_advert_get(
      int unit,
      opennsl_port_t port,
      opennsl_port_ability_t* ability_mask) = 0;

  virtual int opennsl_l3_route_multipath_get(
      int unit,
      opennsl_l3_route_t* the_route,
      opennsl_l3_route_t* path_array,
      int max_path,
      int* path_count) = 0;

  virtual int opennsl_port_queued_count_get(
      int unit,
      opennsl_port_t port,
      uint32* count) = 0;

  virtual int opennsl_l3_info(int unit, opennsl_l3_info_t* l3info) = 0;

  virtual int
  opennsl_port_learn_get(int unit, opennsl_port_t port, uint32* flags) = 0;

  virtual int
  opennsl_port_frame_max_set(int unit, opennsl_port_t port, int size) = 0;

  virtual void opennsl_l3_host_t_init(opennsl_l3_host_t* ip) = 0;

  virtual int opennsl_l3_route_delete_all(
      int unit,
      opennsl_l3_route_t* info) = 0;

  virtual int opennsl_rx_cfg_get(int unit, opennsl_rx_cfg_t* cfg) = 0;

  virtual int opennsl_port_ability_advert_set(
      int unit,
      opennsl_port_t port,
      opennsl_port_ability_t* ability_mask) = 0;

  virtual int opennsl_knet_filter_destroy(int unit, int filter_id) = 0;

  virtual int opennsl_l3_egress_find(
      int unit,
      opennsl_l3_egress_t* egr,
      opennsl_if_t* intf) = 0;

  virtual int opennsl_l3_route_get(int unit, opennsl_l3_route_t* info) = 0;

  virtual int opennsl_cosq_bst_stat_get(
      int unit,
      opennsl_gport_t gport,
      opennsl_cos_queue_t cosq,
      opennsl_bst_stat_id_t bid,
      uint32 options,
      uint64* value) = 0;

  virtual int opennsl_stat_clear(int unit, opennsl_port_t port) = 0;

  virtual int opennsl_l3_route_max_ecmp_set(int unit, int max) = 0;

  virtual int opennsl_switch_event_unregister(
      int unit,
      opennsl_switch_event_cb_t cb,
      void* userdata) = 0;

  virtual int
  opennsl_attach(int unit, char* type, char* subtype, int remunit) = 0;

  virtual int
  opennsl_port_speed_set(int unit, opennsl_port_t port, int speed) = 0;

  virtual int opennsl_l3_egress_destroy(int unit, opennsl_if_t intf) = 0;

  virtual int
  opennsl_port_enable_set(int unit, opennsl_port_t port, int enable) = 0;

  virtual int
  opennsl_l2_addr_delete(int unit, opennsl_mac_t mac, opennsl_vlan_t vid) = 0;

  virtual int opennsl_vlan_port_add(
      int unit,
      opennsl_vlan_t vid,
      opennsl_pbmp_t pbmp,
      opennsl_pbmp_t ubmp) = 0;

  virtual int opennsl_port_selective_get(
      int unit,
      opennsl_port_t port,
      opennsl_port_info_t* info) = 0;

  virtual void opennsl_l2_addr_t_init(
      opennsl_l2_addr_t* l2addr,
      const opennsl_mac_t mac_addr,
      opennsl_vlan_t vid) = 0;

  virtual int opennsl_cosq_bst_profile_get(
      int unit,
      opennsl_gport_t gport,
      opennsl_cos_queue_t cosq,
      opennsl_bst_stat_id_t bid,
      opennsl_cosq_bst_profile_t* profile) = 0;

  virtual int opennsl_l3_egress_ecmp_delete(
      int unit,
      opennsl_l3_egress_ecmp_t* ecmp,
      opennsl_if_t intf) = 0;

  virtual int
  opennsl_port_link_status_get(int unit, opennsl_port_t port, int* status) = 0;

  virtual char* opennsl_port_name(int unit, int port) = 0;

  virtual int
  opennsl_stg_list_destroy(int unit, opennsl_stg_t* list, int count) = 0;

  virtual int opennsl_linkscan_enable_set(int unit, int us) = 0;

  virtual int
  opennsl_port_dtag_mode_set(int unit, opennsl_port_t port, int mode) = 0;

  virtual int opennsl_l3_egress_ecmp_create(
      int unit,
      opennsl_l3_egress_ecmp_t* ecmp,
      int intf_count,
      opennsl_if_t* intf_array) = 0;

  virtual int opennsl_stk_my_modid_get(int unit, int* my_modid) = 0;

  virtual int opennsl_l3_intf_delete(int unit, opennsl_l3_intf_t* intf) = 0;

  virtual int opennsl_l3_egress_multipath_add(
      int unit,
      opennsl_if_t mpintf,
      opennsl_if_t intf) = 0;

  virtual int opennsl_l3_egress_traverse(
      int unit,
      opennsl_l3_egress_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int opennsl_knet_init(int unit) = 0;

  virtual int opennsl_l3_route_add(int unit, opennsl_l3_route_t* info) = 0;

  virtual int opennsl_stat_multi_get(
      int unit,
      opennsl_port_t port,
      int nstat,
      opennsl_stat_val_t* stat_arr,
      uint64* value_arr) = 0;

  virtual int opennsl_l3_intf_get(int unit, opennsl_l3_intf_t* intf) = 0;

  virtual int opennsl_detach(int unit) = 0;

  virtual int opennsl_l2_addr_get(
      int unit,
      opennsl_mac_t mac_addr,
      opennsl_vlan_t vid,
      opennsl_l2_addr_t* l2addr) = 0;

  virtual int opennsl_stat_get(
      int unit,
      opennsl_port_t port,
      opennsl_stat_val_t type,
      uint64* value) = 0;

  virtual void opennsl_port_info_t_init(opennsl_port_info_t* info) = 0;

  virtual int opennsl_stg_stp_set(
      int unit,
      opennsl_stg_t stg,
      opennsl_port_t port,
      int stp_state) = 0;

  virtual int opennsl_port_untagged_vlan_get(
      int unit,
      opennsl_port_t port,
      opennsl_vlan_t* vid_ptr) = 0;

  virtual int opennsl_vlan_default_set(int unit, opennsl_vlan_t vid) = 0;

  virtual int opennsl_linkscan_unregister(
      int unit,
      opennsl_linkscan_handler_t f) = 0;

  virtual int opennsl_l3_egress_ecmp_destroy(
      int unit,
      opennsl_l3_egress_ecmp_t* ecmp) = 0;

  virtual int opennsl_ip6_mask_create(opennsl_ip6_t ip6, int len) = 0;

  virtual int opennsl_rx_start(int unit, opennsl_rx_cfg_t* cfg) = 0;

  virtual int opennsl_l3_egress_create(
      int unit,
      uint32 flags,
      opennsl_l3_egress_t* egr,
      opennsl_if_t* if_id) = 0;

  virtual int opennsl_l3_intf_find(int unit, opennsl_l3_intf_t* intf) = 0;

  virtual int opennsl_stg_list(int unit, opennsl_stg_t** list, int* count) = 0;

  virtual int opennsl_port_vlan_member_get(
      int unit,
      opennsl_port_t port,
      uint32* flags) = 0;

  virtual int
  opennsl_stg_vlan_add(int unit, opennsl_stg_t stg, opennsl_vlan_t vid) = 0;

  virtual void opennsl_knet_netif_t_init(opennsl_knet_netif_t* netif) = 0;

  virtual int
  opennsl_rx_control_get(int unit, opennsl_rx_control_t type, int* arg) = 0;

  virtual int
  opennsl_pkt_flags_init(int unit, opennsl_pkt_t* pkt, uint32 init_flags) = 0;

  virtual int opennsl_l3_egress_multipath_traverse(
      int unit,
      opennsl_l3_egress_multipath_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int opennsl_l3_host_traverse(
      int unit,
      uint32 flags,
      uint32 start,
      uint32 end,
      opennsl_l3_host_traverse_cb cb,
      void* user_data) = 0;

  virtual int opennsl_l3_host_add(int unit, opennsl_l3_host_t* info) = 0;

  virtual int
  opennsl_port_learn_set(int unit, opennsl_port_t port, uint32 flags) = 0;

  virtual int opennsl_l2_station_add(
      int unit,
      int* station_id,
      opennsl_l2_station_t* station) = 0;

  virtual int opennsl_port_phy_modify(
      int unit,
      opennsl_port_t port,
      uint32 flags,
      uint32 phy_reg_addr,
      uint32 phy_data,
      uint32 phy_mask) = 0;

  virtual int opennsl_l3_host_delete_all(int unit, opennsl_l3_host_t* info) = 0;

  virtual int opennsl_l2_addr_add(int unit, opennsl_l2_addr_t* l2addr) = 0;

  virtual int opennsl_cosq_bst_stat_multi_get(
      int unit,
      opennsl_gport_t gport,
      opennsl_cos_queue_t cosq,
      uint32 options,
      int max_values,
      opennsl_bst_stat_id_t* id_list,
      uint64* values) = 0;

  virtual int opennsl_l3_egress_ecmp_find(
      int unit,
      int intf_count,
      opennsl_if_t* intf_array,
      opennsl_l3_egress_ecmp_t* ecmp) = 0;

  virtual int opennsl_pkt_free(int unit, opennsl_pkt_t* pkt) = 0;

  virtual int opennsl_vlan_control_port_set(
      int unit,
      int port,
      opennsl_vlan_control_port_t type,
      int arg) = 0;

  virtual int opennsl_vlan_create(int unit, opennsl_vlan_t vid) = 0;

  virtual int opennsl_attach_check(int unit) = 0;

  virtual int
  opennsl_port_dtag_mode_get(int unit, opennsl_port_t port, int* mode) = 0;

  virtual int opennsl_pkt_alloc(
      int unit,
      int size,
      uint32 flags,
      opennsl_pkt_t** pkt_buf) = 0;

  virtual int opennsl_knet_filter_traverse(
      int unit,
      opennsl_knet_filter_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int
  opennsl_port_speed_max(int unit, opennsl_port_t port, int* speed) = 0;

  virtual int
  opennsl_linkscan_mode_set_pbm(int unit, opennsl_pbmp_t pbm, int mode) = 0;

  virtual void opennsl_l2_station_t_init(opennsl_l2_station_t* addr) = 0;

  virtual int opennsl_port_ability_local_get(
      int unit,
      opennsl_port_t port,
      opennsl_port_ability_t* local_ability_mask) = 0;

  virtual int
  opennsl_port_enable_get(int unit, opennsl_port_t port, int* enable) = 0;

  virtual void opennsl_l3_intf_t_init(opennsl_l3_intf_t* intf) = 0;

  virtual int
  opennsl_port_frame_max_get(int unit, opennsl_port_t port, int* size) = 0;

  virtual int opennsl_cosq_bst_stat_clear(
      int unit,
      opennsl_gport_t gport,
      opennsl_cos_queue_t cosq,
      opennsl_bst_stat_id_t bid) = 0;

  virtual int opennsl_cosq_mapping_set(
      int unit,
      opennsl_cos_t priority,
      opennsl_cos_queue_t cosq) = 0;

  virtual int opennsl_port_selective_set(
      int unit,
      opennsl_port_t port,
      opennsl_port_info_t* info) = 0;

  virtual int opennsl_port_interface_set(
      int unit,
      opennsl_port_t port,
      opennsl_port_if_t intf) = 0;

  virtual int opennsl_l3_route_max_ecmp_get(int unit, int* max) = 0;

  virtual int opennsl_port_interface_get(
      int unit,
      opennsl_port_t port,
      opennsl_port_if_t* intf) = 0;

  virtual int opennsl_l3_egress_multipath_destroy(
      int unit,
      opennsl_if_t mpintf) = 0;

  virtual int opennsl_vlan_gport_delete_all(int unit, opennsl_vlan_t vlan) = 0;

  virtual int opennsl_l3_egress_multipath_delete(
      int unit,
      opennsl_if_t mpintf,
      opennsl_if_t intf) = 0;

  virtual int opennsl_knet_filter_create(
      int unit,
      opennsl_knet_filter_t* filter) = 0;

  virtual int opennsl_switch_control_set(
      int unit,
      opennsl_switch_control_t type,
      int arg) = 0;

  virtual void opennsl_port_ability_t_init(opennsl_port_ability_t* ability) = 0;

  virtual int opennsl_rx_free(int unit, void* pkt_data) = 0;

  virtual int opennsl_rx_register(
      int unit,
      const char* name,
      opennsl_rx_cb_f callback,
      uint8 priority,
      void* cookie,
      uint32 flags) = 0;

  virtual int
  opennsl_port_speed_get(int unit, opennsl_port_t port, int* speed) = 0;

  virtual int opennsl_knet_netif_traverse(
      int unit,
      opennsl_knet_netif_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int opennsl_switch_event_register(
      int unit,
      opennsl_switch_event_cb_t cb,
      void* userdata) = 0;

  virtual int opennsl_linkscan_register(
      int unit,
      opennsl_linkscan_handler_t f) = 0;

  virtual int opennsl_switch_control_get(
      int unit,
      opennsl_switch_control_t type,
      int* arg) = 0;
};

class OpenNSLMock : public OpenNSLInterface {
 public:
  static void resetMock(std::unique_ptr<OpenNSLMock>) noexcept;
  static OpenNSLMock* getMock() noexcept;
  static void destroyMock() noexcept;

  MOCK_METHOD3(
      opennsl_l3_egress_traverse,
      int(int, opennsl_l3_egress_traverse_cb, void*));
  MOCK_METHOD3(
      opennsl_knet_netif_traverse,
      int(int, opennsl_knet_netif_traverse_cb, void*));
  MOCK_METHOD3(opennsl_linkscan_mode_get, int(int, opennsl_port_t, int*));
  MOCK_METHOD2(opennsl_rx_cfg_get, int(int, opennsl_rx_cfg_t*));
  MOCK_METHOD3(opennsl_vlan_list, int(int, opennsl_vlan_data_t**, int*));
  MOCK_METHOD1(opennsl_l3_egress_t_init, void(opennsl_l3_egress_t*));
  MOCK_METHOD3(opennsl_rx_control_set, int(int, opennsl_rx_control_t, int));
  MOCK_METHOD3(
      opennsl_l3_egress_find,
      int(int, opennsl_l3_egress_t*, opennsl_if_t*));
  MOCK_METHOD3(opennsl_pkt_flags_init, int(int, opennsl_pkt_t*, uint32));
  MOCK_METHOD2(opennsl_rx_free, int(int, void*));
  MOCK_METHOD1(opennsl_knet_init, int(int));
  MOCK_METHOD4(opennsl_pkt_alloc, int(int, int, uint32, opennsl_pkt_t**));
  MOCK_METHOD2(opennsl_linkscan_register, int(int, opennsl_linkscan_handler_t));
  MOCK_METHOD2(
      opennsl_l3_route_delete_by_interface,
      int(int, opennsl_l3_route_t*));
  MOCK_METHOD3(
      opennsl_switch_event_unregister,
      int(int, opennsl_switch_event_cb_t, void*));
  MOCK_METHOD1(opennsl_trunk_detach, int(int));
  MOCK_METHOD1(opennsl_trunk_info_t_init, void(opennsl_trunk_info_t*));
  MOCK_METHOD4(
      opennsl_trunk_find,
      int(int, opennsl_module_t, opennsl_port_t, opennsl_trunk_t*));
  MOCK_METHOD6(
      opennsl_trunk_get,
      int(int,
          opennsl_trunk_t,
          opennsl_trunk_info_t*,
          int,
          opennsl_trunk_member_t*,
          int*));
  MOCK_METHOD3(opennsl_trunk_create, int(int, uint32, opennsl_trunk_t*));
  MOCK_METHOD2(
      opennsl_trunk_chip_info_get,
      int(int, opennsl_trunk_chip_info_t*));
  MOCK_METHOD1(opennsl_trunk_member_t_init, void(opennsl_trunk_member_t*));
  MOCK_METHOD3(
      opennsl_trunk_member_delete,
      int(int, opennsl_trunk_t, opennsl_trunk_member_t*));
  MOCK_METHOD3(opennsl_trunk_psc_set, int(int, opennsl_trunk_t, int));
  MOCK_METHOD5(
      opennsl_trunk_set,
      int(int,
          opennsl_trunk_t,
          opennsl_trunk_info_t*,
          int,
          opennsl_trunk_member_t*));
  MOCK_METHOD3(
      opennsl_trunk_member_add,
      int(int, opennsl_trunk_t, opennsl_trunk_member_t*));
  MOCK_METHOD2(opennsl_trunk_destroy, int(int, opennsl_trunk_t));
  MOCK_METHOD1(opennsl_trunk_init, int(int));
  MOCK_METHOD2(opennsl_l3_host_delete_all, int(int, opennsl_l3_host_t*));
  MOCK_METHOD3(
      opennsl_port_untagged_vlan_get,
      int(int, opennsl_port_t, opennsl_vlan_t*));
  MOCK_METHOD2(opennsl_l3_intf_find, int(int, opennsl_l3_intf_t*));
  MOCK_METHOD2(opennsl_l3_intf_delete, int(int, opennsl_l3_intf_t*));
  MOCK_METHOD1(opennsl_vlan_destroy_all, int(int));
  MOCK_METHOD3(opennsl_vlan_list_destroy, int(int, opennsl_vlan_data_t*, int));
  MOCK_METHOD3(
      opennsl_port_untagged_vlan_set,
      int(int, opennsl_port_t, opennsl_vlan_t));
  MOCK_METHOD3(opennsl_port_frame_max_set, int(int, opennsl_port_t, int));
  MOCK_METHOD3(opennsl_rx_control_get, int(int, opennsl_rx_control_t, int*));
  MOCK_METHOD3(opennsl_stg_list_destroy, int(int, opennsl_stg_t*, int));
  MOCK_METHOD3(
      opennsl_cosq_mapping_get,
      int(int, opennsl_cos_t, opennsl_cos_queue_t*));
  MOCK_METHOD4(
      opennsl_switch_control_port_get,
      int(int, opennsl_port_t, opennsl_switch_control_t, int*));
  MOCK_METHOD3(
      opennsl_l3_egress_ecmp_traverse,
      int(int, opennsl_l3_egress_ecmp_traverse_cb, void*));
  MOCK_METHOD3(
      opennsl_cosq_mapping_set,
      int(int, opennsl_cos_t, opennsl_cos_queue_t));
  MOCK_METHOD3(
      opennsl_l3_egress_ecmp_delete,
      int(int, opennsl_l3_egress_ecmp_t*, opennsl_if_t));
  MOCK_METHOD1(opennsl_detach, int(int));
  MOCK_METHOD7(
      opennsl_cosq_bst_stat_multi_get,
      int(int,
          opennsl_gport_t,
          opennsl_cos_queue_t,
          uint32,
          int,
          opennsl_bst_stat_id_t*,
          uint64*));
  MOCK_METHOD3(
      opennsl_port_ability_advert_get,
      int(int, opennsl_port_t, opennsl_port_ability_t*));
  MOCK_METHOD2(opennsl_l2_age_timer_get, int(int, int*));
  MOCK_METHOD3(
      opennsl_port_queued_count_get,
      int(int, opennsl_port_t, uint32*));
  MOCK_METHOD2(opennsl_knet_filter_create, int(int, opennsl_knet_filter_t*));
  MOCK_METHOD2(opennsl_vlan_create, int(int, opennsl_vlan_t));
  MOCK_METHOD2(
      opennsl_l3_egress_ecmp_destroy,
      int(int, opennsl_l3_egress_ecmp_t*));
  MOCK_METHOD3(opennsl_linkscan_mode_set_pbm, int(int, opennsl_pbmp_t, int));
  MOCK_METHOD1(opennsl_l3_host_t_init, void(opennsl_l3_host_t*));
  MOCK_METHOD2(opennsl_l3_host_find, int(int, opennsl_l3_host_t*));
  MOCK_METHOD6(
      opennsl_switch_pkt_trace_info_get,
      int(int, uint32, uint8, int, uint8*, opennsl_switch_pkt_trace_info_t*));
  MOCK_METHOD2(opennsl_port_config_get, int(int, opennsl_port_config_t*));
  MOCK_METHOD2(opennsl_l3_route_delete_all, int(int, opennsl_l3_route_t*));
  MOCK_METHOD2(opennsl_l2_addr_add, int(int, opennsl_l2_addr_t*));
  MOCK_METHOD2(opennsl_l3_host_add, int(int, opennsl_l3_host_t*));
  MOCK_METHOD5(
      opennsl_l3_ecmp_get,
      int(int,
          opennsl_l3_egress_ecmp_t*,
          int,
          opennsl_l3_ecmp_member_t*,
          int*));
  MOCK_METHOD1(opennsl_port_info_t_init, void(opennsl_port_info_t*));
  MOCK_METHOD3(opennsl_port_enable_set, int(int, opennsl_port_t, int));
  MOCK_METHOD3(opennsl_port_stat_enable_set, int(int, opennsl_gport_t, int));
  MOCK_METHOD1(opennsl_ip_mask_create, opennsl_ip_t(int));
  MOCK_METHOD3(
      opennsl_l3_egress_multipath_add,
      int(int, opennsl_if_t, opennsl_if_t));
  MOCK_METHOD6(
      opennsl_cosq_bst_stat_get,
      int(int,
          opennsl_gport_t,
          opennsl_cos_queue_t,
          opennsl_bst_stat_id_t,
          uint32,
          uint64*));
  MOCK_METHOD2(opennsl_l3_route_get, int(int, opennsl_l3_route_t*));
  MOCK_METHOD3(
      opennsl_l3_egress_multipath_traverse,
      int(int, opennsl_l3_egress_multipath_traverse_cb, void*));
  MOCK_METHOD3(opennsl_port_vlan_member_set, int(int, opennsl_port_t, uint32));
  MOCK_METHOD4(
      opennsl_l3_egress_multipath_find,
      int(int, int, opennsl_if_t*, opennsl_if_t*));
  MOCK_METHOD2(opennsl_l3_intf_get, int(int, opennsl_l3_intf_t*));
  MOCK_METHOD4(
      opennsl_l3_egress_create,
      int(int, uint32, opennsl_l3_egress_t*, opennsl_if_t*));
  MOCK_METHOD6(
      opennsl_l3_host_traverse,
      int(int, uint32, uint32, uint32, opennsl_l3_host_traverse_cb, void*));
  MOCK_METHOD2(
      opennsl_l3_host_delete_by_interface,
      int(int, opennsl_l3_host_t*));
  MOCK_METHOD2(
      opennsl_linkscan_unregister,
      int(int, opennsl_linkscan_handler_t));
  MOCK_METHOD1(opennsl_knet_filter_t_init, void(opennsl_knet_filter_t*));
  MOCK_METHOD3(opennsl_rx_unregister, int(int, opennsl_rx_cb_f, uint8));
  MOCK_METHOD3(opennsl_l2_traverse, int(int, opennsl_l2_traverse_cb, void*));
  MOCK_METHOD5(
      opennsl_l3_egress_multipath_get,
      int(int, opennsl_if_t, int, opennsl_if_t*, int*));
  MOCK_METHOD6(
      opennsl_rx_register,
      int(int, const char*, opennsl_rx_cb_f, uint8, void*, uint32));
  MOCK_METHOD2(opennsl_ip6_mask_create, int(opennsl_ip6_t, int));
  MOCK_METHOD6(
      opennsl_port_phy_modify,
      int(int, opennsl_port_t, uint32, uint32, uint32, uint32));
  MOCK_METHOD2(opennsl_rx_start, int(int, opennsl_rx_cfg_t*));
  MOCK_METHOD3(opennsl_l2_station_get, int(int, int, opennsl_l2_station_t*));
  MOCK_METHOD2(opennsl_l3_egress_multipath_destroy, int(int, opennsl_if_t));
  MOCK_METHOD1(opennsl_l3_egress_ecmp_t_init, void(opennsl_l3_egress_ecmp_t*));
  MOCK_METHOD5(
      opennsl_l3_egress_multipath_create,
      int(int, uint32, int, opennsl_if_t*, opennsl_if_t*));
  MOCK_METHOD4(
      opennsl_stat_get,
      int(int, opennsl_port_t, opennsl_stat_val_t, uint64*));
  MOCK_METHOD2(opennsl_stk_my_modid_get, int(int, int*));
  MOCK_METHOD2(opennsl_pkt_free, int(int, opennsl_pkt_t*));
  MOCK_METHOD3(opennsl_tx, int(int, opennsl_pkt_t*, void*));
  MOCK_METHOD2(opennsl_l3_egress_destroy, int(int, opennsl_if_t));
  MOCK_METHOD4(
      opennsl_port_control_get,
      int(int, opennsl_port_t, opennsl_port_control_t, int*));
  MOCK_METHOD3(
      opennsl_switch_control_get,
      int(int, opennsl_switch_control_t, int*));
  MOCK_METHOD2(opennsl_linkscan_enable_set, int(int, int));
  MOCK_METHOD3(opennsl_port_vlan_member_get, int(int, opennsl_port_t, uint32*));
  MOCK_METHOD3(opennsl_l2_station_add, int(int, int*, opennsl_l2_station_t*));
  MOCK_METHOD3(opennsl_port_enable_get, int(int, opennsl_port_t, int*));
  MOCK_METHOD2(opennsl_l3_host_delete, int(int, opennsl_l3_host_t*));
  MOCK_METHOD2(opennsl_l3_route_max_ecmp_get, int(int, int*));
  MOCK_METHOD1(opennsl_l3_intf_t_init, void(opennsl_l3_intf_t*));
  MOCK_METHOD3(
      opennsl_l3_egress_get,
      int(int, opennsl_if_t, opennsl_l3_egress_t*));
  MOCK_METHOD2(opennsl_linkscan_enable_get, int(int, int*));
  MOCK_METHOD2(opennsl_l3_intf_find_vlan, int(int, opennsl_l3_intf_t*));
  MOCK_METHOD1(opennsl_linkscan_detach, int(int));
  MOCK_METHOD4(
      opennsl_vlan_port_add,
      int(int, opennsl_vlan_t, opennsl_pbmp_t, opennsl_pbmp_t));
  MOCK_METHOD3(
      opennsl_port_gport_get,
      int(int, opennsl_port_t, opennsl_gport_t*));
  MOCK_METHOD2(opennsl_cosq_bst_stat_sync, int(int, opennsl_bst_stat_id_t));
  MOCK_METHOD1(opennsl_port_ability_t_init, void(opennsl_port_ability_t*));
  MOCK_METHOD3(opennsl_port_speed_max, int(int, opennsl_port_t, int*));
  MOCK_METHOD5(
      opennsl_cosq_bst_profile_set,
      int(int,
          opennsl_gport_t,
          opennsl_cos_queue_t,
          opennsl_bst_stat_id_t,
          opennsl_cosq_bst_profile_t*));
  MOCK_METHOD2(opennsl_vlan_destroy, int(int, opennsl_vlan_t));
  MOCK_METHOD3(
      opennsl_port_local_get,
      int(int, opennsl_gport_t, opennsl_port_t*));
  MOCK_METHOD4(
      opennsl_switch_control_port_set,
      int(int, opennsl_port_t, opennsl_switch_control_t, int));
  MOCK_METHOD3(
      opennsl_switch_event_register,
      int(int, opennsl_switch_event_cb_t, void*));
  MOCK_METHOD2(opennsl_knet_netif_create, int(int, opennsl_knet_netif_t*));
  MOCK_METHOD2(opennsl_l3_intf_create, int(int, opennsl_l3_intf_t*));
  MOCK_METHOD3(
      opennsl_port_ability_advert_set,
      int(int, opennsl_port_t, opennsl_port_ability_t*));
  MOCK_METHOD1(opennsl_knet_netif_t_init, void(opennsl_knet_netif_t*));
  MOCK_METHOD3(
      opennsl_l2_addr_t_init,
      void(opennsl_l2_addr_t*, const opennsl_mac_t mac_addr, opennsl_vlan_t));
  MOCK_METHOD2(opennsl_l3_route_delete, int(int, opennsl_l3_route_t*));
  MOCK_METHOD2(opennsl_stg_create, int(int, opennsl_stg_t*));
  MOCK_METHOD1(opennsl_l3_init, int(int));
  MOCK_METHOD4(
      opennsl_port_control_set,
      int(int, opennsl_port_t, opennsl_port_control_t, int));
  MOCK_METHOD3(
      opennsl_switch_control_set,
      int(int, opennsl_switch_control_t, int));
  MOCK_METHOD1(opennsl_l3_route_t_init, void(opennsl_l3_route_t*));
  MOCK_METHOD4(
      opennsl_l3_egress_ecmp_find,
      int(int, int, opennsl_if_t*, opennsl_l3_egress_ecmp_t*));
  MOCK_METHOD1(opennsl_l2_station_t_init, void(opennsl_l2_station_t*));
  MOCK_METHOD2(opennsl_stg_default_set, int(int, opennsl_stg_t));
  MOCK_METHOD3(opennsl_port_dtag_mode_set, int(int, opennsl_port_t, int));
  MOCK_METHOD3(opennsl_port_link_status_get, int(int, opennsl_port_t, int*));
  MOCK_METHOD3(opennsl_port_learn_set, int(int, opennsl_port_t, uint32));
  MOCK_METHOD3(opennsl_stg_vlan_add, int(int, opennsl_stg_t, opennsl_vlan_t));
  MOCK_METHOD3(opennsl_port_dtag_mode_get, int(int, opennsl_port_t, int*));
  MOCK_METHOD5(
      opennsl_stat_multi_get,
      int(int, opennsl_port_t, int, opennsl_stat_val_t*, uint64*));
  MOCK_METHOD2(opennsl_l3_info, int(int, opennsl_l3_info_t*));
  MOCK_METHOD2(opennsl_knet_netif_destroy, int(int, int));
  MOCK_METHOD3(opennsl_l2_addr_delete, int(int, opennsl_mac_t, opennsl_vlan_t));
  MOCK_METHOD4(
      opennsl_vlan_control_port_set,
      int(int, int, opennsl_vlan_control_port_t, int));
  MOCK_METHOD2(opennsl_info_get, int(int, opennsl_info_t*));
  MOCK_METHOD5(
      opennsl_cosq_bst_profile_get,
      int(int,
          opennsl_gport_t,
          opennsl_cos_queue_t,
          opennsl_bst_stat_id_t,
          opennsl_cosq_bst_profile_t*));
  MOCK_METHOD3(
      opennsl_port_interface_set,
      int(int, opennsl_port_t, opennsl_port_if_t));
  MOCK_METHOD2(opennsl_vlan_default_get, int(int, opennsl_vlan_t*));
  MOCK_METHOD3(opennsl_linkscan_mode_set, int(int, opennsl_port_t, int));
  MOCK_METHOD3(
      opennsl_knet_filter_traverse,
      int(int, opennsl_knet_filter_traverse_cb, void*));
  MOCK_METHOD3(opennsl_port_speed_set, int(int, opennsl_port_t, int));
  MOCK_METHOD4(
      opennsl_stg_stp_set,
      int(int, opennsl_stg_t, opennsl_port_t, int));
  MOCK_METHOD2(opennsl_l3_route_add, int(int, opennsl_l3_route_t*));
  MOCK_METHOD2(opennsl_vlan_default_set, int(int, opennsl_vlan_t));
  MOCK_METHOD3(opennsl_port_speed_get, int(int, opennsl_port_t, int*));
  MOCK_METHOD3(opennsl_port_learn_get, int(int, opennsl_port_t, uint32*));
  MOCK_METHOD2(opennsl_stat_clear, int(int, opennsl_port_t));
  MOCK_METHOD3(
      opennsl_l3_egress_multipath_delete,
      int(int, opennsl_if_t, opennsl_if_t));
  MOCK_METHOD2(opennsl_l2_station_delete, int(int, int));
  MOCK_METHOD4(opennsl_attach, int(int, char*, char*, int));
  MOCK_METHOD2(opennsl_l2_age_timer_set, int(int, int));
  MOCK_METHOD3(
      opennsl_port_selective_get,
      int(int, opennsl_port_t, opennsl_port_info_t*));
  MOCK_METHOD2(opennsl_port_name, char*(int, int));
  MOCK_METHOD5(
      opennsl_l3_route_multipath_get,
      int(int, opennsl_l3_route_t*, opennsl_l3_route_t*, int, int*));
  MOCK_METHOD3(opennsl_stg_list, int(int, opennsl_stg_t**, int*));
  MOCK_METHOD4(
      opennsl_cosq_bst_stat_clear,
      int(int, opennsl_gport_t, opennsl_cos_queue_t, opennsl_bst_stat_id_t));
  MOCK_METHOD1(opennsl_attach_check, int(int));
  MOCK_METHOD3(
      opennsl_port_ability_local_get,
      int(int, opennsl_port_t, opennsl_port_ability_t*));
  MOCK_METHOD2(opennsl_vlan_gport_delete_all, int(int, opennsl_vlan_t));
  MOCK_METHOD2(opennsl_stg_destroy, int(int, opennsl_stg_t));
  MOCK_METHOD2(opennsl_stg_default_get, int(int, opennsl_stg_t*));
  MOCK_METHOD4(
      opennsl_l2_addr_get,
      int(int, opennsl_mac_t, opennsl_vlan_t, opennsl_l2_addr_t*));
  MOCK_METHOD3(
      opennsl_port_interface_get,
      int(int, opennsl_port_t, opennsl_port_if_t*));
  MOCK_METHOD4(
      opennsl_stg_stp_get,
      int(int, opennsl_stg_t, opennsl_port_t, int*));
  MOCK_METHOD6(
      opennsl_l3_route_traverse,
      int(int, uint32, uint32, uint32, opennsl_l3_route_traverse_cb, void*));
  MOCK_METHOD3(
      opennsl_port_selective_set,
      int(int, opennsl_port_t, opennsl_port_info_t*));
  MOCK_METHOD3(opennsl_port_frame_max_get, int(int, opennsl_port_t, int*));
  MOCK_METHOD1(opennsl_l3_info_t_init, void(opennsl_l3_info_t*));
  MOCK_METHOD4(
      opennsl_l3_egress_ecmp_create,
      int(int, opennsl_l3_egress_ecmp_t*, int, opennsl_if_t*));
  MOCK_METHOD1(opennsl_attach_max, int(int*));
  MOCK_METHOD3(
      opennsl_l3_egress_ecmp_add,
      int(int, opennsl_l3_egress_ecmp_t*, opennsl_if_t));
  MOCK_METHOD2(opennsl_rx_stop, int(int, opennsl_rx_cfg_t*));
  MOCK_METHOD2(opennsl_l3_route_max_ecmp_set, int(int, int));
  MOCK_METHOD3(
      opennsl_vlan_port_remove,
      int(int, opennsl_vlan_t, opennsl_pbmp_t));
  MOCK_METHOD2(opennsl_knet_filter_destroy, int(int, int));
};

}} // namespace facebook::fboss

extern "C" {

int __wrap_opennsl_l3_egress_traverse(
    int unit,
    opennsl_l3_egress_traverse_cb trav_fn,
    void* user_data);

int __wrap_opennsl_knet_netif_traverse(
    int unit,
    opennsl_knet_netif_traverse_cb trav_fn,
    void* user_data);

int __wrap_opennsl_linkscan_mode_get(int unit, opennsl_port_t port, int* mode);

int __wrap_opennsl_rx_cfg_get(int unit, opennsl_rx_cfg_t* cfg);

int __wrap_opennsl_vlan_list(
    int unit,
    opennsl_vlan_data_t** listp,
    int* countp);

void __wrap_opennsl_l3_egress_t_init(opennsl_l3_egress_t* egr);

int __wrap_opennsl_rx_control_set(int unit, opennsl_rx_control_t type, int arg);

int __wrap_opennsl_l3_egress_find(
    int unit,
    opennsl_l3_egress_t* egr,
    opennsl_if_t* intf);

int __wrap_opennsl_pkt_flags_init(
    int unit,
    opennsl_pkt_t* pkt,
    uint32 init_flags);

int __wrap_opennsl_rx_free(int unit, void* pkt_data);

int __wrap_opennsl_knet_init(int unit);

int __wrap_opennsl_pkt_alloc(
    int unit,
    int size,
    uint32 flags,
    opennsl_pkt_t** pkt_buf);

int __wrap_opennsl_linkscan_register(int unit, opennsl_linkscan_handler_t f);

int __wrap_opennsl_l3_route_delete_by_interface(
    int unit,
    opennsl_l3_route_t* info);

int __wrap_opennsl_switch_event_unregister(
    int unit,
    opennsl_switch_event_cb_t cb,
    void* userdata);

int __wrap_opennsl_trunk_detach(int unit);

void __wrap_opennsl_trunk_info_t_init(opennsl_trunk_info_t* trunk_info);

int __wrap_opennsl_trunk_find(
    int unit,
    opennsl_module_t modid,
    opennsl_port_t port,
    opennsl_trunk_t* tid);

int __wrap_opennsl_trunk_get(
    int unit,
    opennsl_trunk_t tid,
    opennsl_trunk_info_t* t_data,
    int member_max,
    opennsl_trunk_member_t* member_array,
    int* member_count);

int __wrap_opennsl_trunk_create(int unit, uint32 flags, opennsl_trunk_t* tid);

int __wrap_opennsl_trunk_chip_info_get(
    int unit,
    opennsl_trunk_chip_info_t* ta_info);

void __wrap_opennsl_trunk_member_t_init(opennsl_trunk_member_t* trunk_member);

int __wrap_opennsl_trunk_member_delete(
    int unit,
    opennsl_trunk_t tid,
    opennsl_trunk_member_t* member);

int __wrap_opennsl_trunk_psc_set(int unit, opennsl_trunk_t tid, int psc);

int __wrap_opennsl_trunk_set(
    int unit,
    opennsl_trunk_t tid,
    opennsl_trunk_info_t* trunk_info,
    int member_count,
    opennsl_trunk_member_t* member_array);

int __wrap_opennsl_trunk_member_add(
    int unit,
    opennsl_trunk_t tid,
    opennsl_trunk_member_t* member);

int __wrap_opennsl_trunk_destroy(int unit, opennsl_trunk_t tid);

int __wrap_opennsl_trunk_init(int unit);

int __wrap_opennsl_l3_host_delete_all(int unit, opennsl_l3_host_t* info);

int __wrap_opennsl_port_untagged_vlan_get(
    int unit,
    opennsl_port_t port,
    opennsl_vlan_t* vid_ptr);

int __wrap_opennsl_l3_intf_find(int unit, opennsl_l3_intf_t* intf);

int __wrap_opennsl_l3_intf_delete(int unit, opennsl_l3_intf_t* intf);

int __wrap_opennsl_vlan_destroy_all(int unit);

int __wrap_opennsl_vlan_list_destroy(
    int unit,
    opennsl_vlan_data_t* list,
    int count);

int __wrap_opennsl_port_untagged_vlan_set(
    int unit,
    opennsl_port_t port,
    opennsl_vlan_t vid);

int __wrap_opennsl_port_frame_max_set(int unit, opennsl_port_t port, int size);

int __wrap_opennsl_rx_control_get(
    int unit,
    opennsl_rx_control_t type,
    int* arg);

int __wrap_opennsl_stg_list_destroy(int unit, opennsl_stg_t* list, int count);

int __wrap_opennsl_cosq_mapping_get(
    int unit,
    opennsl_cos_t priority,
    opennsl_cos_queue_t* cosq);

int __wrap_opennsl_switch_control_port_get(
    int unit,
    opennsl_port_t port,
    opennsl_switch_control_t type,
    int* arg);

int __wrap_opennsl_l3_egress_ecmp_traverse(
    int unit,
    opennsl_l3_egress_ecmp_traverse_cb trav_fn,
    void* user_data);

int __wrap_opennsl_cosq_mapping_set(
    int unit,
    opennsl_cos_t priority,
    opennsl_cos_queue_t cosq);

int __wrap_opennsl_l3_egress_ecmp_delete(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp,
    opennsl_if_t intf);

int __wrap_opennsl_detach(int unit);

int __wrap_opennsl_cosq_bst_stat_multi_get(
    int unit,
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosq,
    uint32 options,
    int max_values,
    opennsl_bst_stat_id_t* id_list,
    uint64* values);

int __wrap_opennsl_port_ability_advert_get(
    int unit,
    opennsl_port_t port,
    opennsl_port_ability_t* ability_mask);

int __wrap_opennsl_l2_age_timer_get(int unit, int* age_seconds);

int __wrap_opennsl_port_queued_count_get(
    int unit,
    opennsl_port_t port,
    uint32* count);

int __wrap_opennsl_knet_filter_create(int unit, opennsl_knet_filter_t* filter);

int __wrap_opennsl_vlan_create(int unit, opennsl_vlan_t vid);

int __wrap_opennsl_l3_egress_ecmp_destroy(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp);

int __wrap_opennsl_linkscan_mode_set_pbm(
    int unit,
    opennsl_pbmp_t pbm,
    int mode);

void __wrap_opennsl_l3_host_t_init(opennsl_l3_host_t* ip);

int __wrap_opennsl_l3_host_find(int unit, opennsl_l3_host_t* info);

int __wrap_opennsl_switch_pkt_trace_info_get(
    int unit,
    uint32 options,
    uint8 port,
    int len,
    uint8* data,
    opennsl_switch_pkt_trace_info_t* pkt_trace_info);

int __wrap_opennsl_port_config_get(int unit, opennsl_port_config_t* config);

int __wrap_opennsl_l3_route_delete_all(int unit, opennsl_l3_route_t* info);

int __wrap_opennsl_l2_addr_add(int unit, opennsl_l2_addr_t* l2addr);

int __wrap_opennsl_l3_host_add(int unit, opennsl_l3_host_t* info);

int __wrap_opennsl_l3_ecmp_get(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp_info,
    int ecmp_member_size,
    opennsl_l3_ecmp_member_t* ecmp_member_array,
    int* ecmp_member_count);

void __wrap_opennsl_port_info_t_init(opennsl_port_info_t* info);

int __wrap_opennsl_port_enable_set(int unit, opennsl_port_t port, int enable);

int __wrap_opennsl_port_stat_enable_set(
    int unit,
    opennsl_gport_t port,
    int enable);

opennsl_ip_t __wrap_opennsl_ip_mask_create(int len);

int __wrap_opennsl_l3_egress_multipath_add(
    int unit,
    opennsl_if_t mpintf,
    opennsl_if_t intf);

int __wrap_opennsl_cosq_bst_stat_get(
    int unit,
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosq,
    opennsl_bst_stat_id_t bid,
    uint32 options,
    uint64* value);

int __wrap_opennsl_l3_route_get(int unit, opennsl_l3_route_t* info);

int __wrap_opennsl_l3_egress_multipath_traverse(
    int unit,
    opennsl_l3_egress_multipath_traverse_cb trav_fn,
    void* user_data);

int __wrap_opennsl_port_vlan_member_set(
    int unit,
    opennsl_port_t port,
    uint32 flags);

int __wrap_opennsl_l3_egress_multipath_find(
    int unit,
    int intf_count,
    opennsl_if_t* intf_array,
    opennsl_if_t* mpintf);

int __wrap_opennsl_l3_intf_get(int unit, opennsl_l3_intf_t* intf);

int __wrap_opennsl_l3_egress_create(
    int unit,
    uint32 flags,
    opennsl_l3_egress_t* egr,
    opennsl_if_t* if_id);

int __wrap_opennsl_l3_host_traverse(
    int unit,
    uint32 flags,
    uint32 start,
    uint32 end,
    opennsl_l3_host_traverse_cb cb,
    void* user_data);

int __wrap_opennsl_l3_host_delete_by_interface(
    int unit,
    opennsl_l3_host_t* info);

int __wrap_opennsl_linkscan_unregister(int unit, opennsl_linkscan_handler_t f);

void __wrap_opennsl_knet_filter_t_init(opennsl_knet_filter_t* filter);

int __wrap_opennsl_rx_unregister(
    int unit,
    opennsl_rx_cb_f callback,
    uint8 priority);

int __wrap_opennsl_l2_traverse(
    int unit,
    opennsl_l2_traverse_cb trav_fn,
    void* user_data);

int __wrap_opennsl_l3_egress_multipath_get(
    int unit,
    opennsl_if_t mpintf,
    int intf_size,
    opennsl_if_t* intf_array,
    int* intf_count);

int __wrap_opennsl_rx_register(
    int unit,
    const char* name,
    opennsl_rx_cb_f callback,
    uint8 priority,
    void* cookie,
    uint32 flags);

int __wrap_opennsl_ip6_mask_create(opennsl_ip6_t ip6, int len);

int __wrap_opennsl_port_phy_modify(
    int unit,
    opennsl_port_t port,
    uint32 flags,
    uint32 phy_reg_addr,
    uint32 phy_data,
    uint32 phy_mask);

int __wrap_opennsl_rx_start(int unit, opennsl_rx_cfg_t* cfg);

int __wrap_opennsl_l2_station_get(
    int unit,
    int station_id,
    opennsl_l2_station_t* station);

int __wrap_opennsl_l3_egress_multipath_destroy(int unit, opennsl_if_t mpintf);

void __wrap_opennsl_l3_egress_ecmp_t_init(opennsl_l3_egress_ecmp_t* ecmp);

int __wrap_opennsl_l3_egress_multipath_create(
    int unit,
    uint32 flags,
    int intf_count,
    opennsl_if_t* intf_array,
    opennsl_if_t* mpintf);

int __wrap_opennsl_stat_get(
    int unit,
    opennsl_port_t port,
    opennsl_stat_val_t type,
    uint64* value);

int __wrap_opennsl_stk_my_modid_get(int unit, int* my_modid);

int __wrap_opennsl_pkt_free(int unit, opennsl_pkt_t* pkt);

int __wrap_opennsl_tx(int unit, opennsl_pkt_t* tx_pkt, void* cookie);

int __wrap_opennsl_l3_egress_destroy(int unit, opennsl_if_t intf);

int __wrap_opennsl_port_control_get(
    int unit,
    opennsl_port_t port,
    opennsl_port_control_t type,
    int* value);

int __wrap_opennsl_switch_control_get(
    int unit,
    opennsl_switch_control_t type,
    int* arg);

int __wrap_opennsl_linkscan_enable_set(int unit, int us);

int __wrap_opennsl_port_vlan_member_get(
    int unit,
    opennsl_port_t port,
    uint32* flags);

int __wrap_opennsl_l2_station_add(
    int unit,
    int* station_id,
    opennsl_l2_station_t* station);

int __wrap_opennsl_port_enable_get(int unit, opennsl_port_t port, int* enable);

int __wrap_opennsl_l3_host_delete(int unit, opennsl_l3_host_t* ip_addr);

int __wrap_opennsl_l3_route_max_ecmp_get(int unit, int* max);

void __wrap_opennsl_l3_intf_t_init(opennsl_l3_intf_t* intf);

int __wrap_opennsl_l3_egress_get(
    int unit,
    opennsl_if_t intf,
    opennsl_l3_egress_t* egr);

int __wrap_opennsl_linkscan_enable_get(int unit, int* us);

int __wrap_opennsl_l3_intf_find_vlan(int unit, opennsl_l3_intf_t* intf);

int __wrap_opennsl_linkscan_detach(int unit);

int __wrap_opennsl_vlan_port_add(
    int unit,
    opennsl_vlan_t vid,
    opennsl_pbmp_t pbmp,
    opennsl_pbmp_t ubmp);

int __wrap_opennsl_port_gport_get(
    int unit,
    opennsl_port_t port,
    opennsl_gport_t* gport);

int __wrap_opennsl_cosq_bst_stat_sync(int unit, opennsl_bst_stat_id_t bid);

void __wrap_opennsl_port_ability_t_init(opennsl_port_ability_t* ability);

int __wrap_opennsl_port_speed_max(int unit, opennsl_port_t port, int* speed);

int __wrap_opennsl_cosq_bst_profile_set(
    int unit,
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosq,
    opennsl_bst_stat_id_t bid,
    opennsl_cosq_bst_profile_t* profile);

int __wrap_opennsl_vlan_destroy(int unit, opennsl_vlan_t vid);

int __wrap_opennsl_port_local_get(
    int unit,
    opennsl_gport_t gport,
    opennsl_port_t* local_port);

int __wrap_opennsl_switch_control_port_set(
    int unit,
    opennsl_port_t port,
    opennsl_switch_control_t type,
    int arg);

int __wrap_opennsl_switch_event_register(
    int unit,
    opennsl_switch_event_cb_t cb,
    void* userdata);

int __wrap_opennsl_knet_netif_create(int unit, opennsl_knet_netif_t* netif);

int __wrap_opennsl_l3_intf_create(int unit, opennsl_l3_intf_t* intf);

int __wrap_opennsl_port_ability_advert_set(
    int unit,
    opennsl_port_t port,
    opennsl_port_ability_t* ability_mask);

void __wrap_opennsl_knet_netif_t_init(opennsl_knet_netif_t* netif);

void __wrap_opennsl_l2_addr_t_init(
    opennsl_l2_addr_t* l2addr,
    const opennsl_mac_t mac_addr,
    opennsl_vlan_t vid);

int __wrap_opennsl_l3_route_delete(int unit, opennsl_l3_route_t* info);

int __wrap_opennsl_stg_create(int unit, opennsl_stg_t* stg_ptr);

int __wrap_opennsl_l3_init(int unit);

int __wrap_opennsl_port_control_set(
    int unit,
    opennsl_port_t port,
    opennsl_port_control_t type,
    int value);

int __wrap_opennsl_switch_control_set(
    int unit,
    opennsl_switch_control_t type,
    int arg);

void __wrap_opennsl_l3_route_t_init(opennsl_l3_route_t* route);

int __wrap_opennsl_l3_egress_ecmp_find(
    int unit,
    int intf_count,
    opennsl_if_t* intf_array,
    opennsl_l3_egress_ecmp_t* ecmp);

void __wrap_opennsl_l2_station_t_init(opennsl_l2_station_t* addr);

int __wrap_opennsl_stg_default_set(int unit, opennsl_stg_t stg);

int __wrap_opennsl_port_dtag_mode_set(int unit, opennsl_port_t port, int mode);

int __wrap_opennsl_port_link_status_get(
    int unit,
    opennsl_port_t port,
    int* status);

int __wrap_opennsl_port_learn_set(int unit, opennsl_port_t port, uint32 flags);

int __wrap_opennsl_stg_vlan_add(
    int unit,
    opennsl_stg_t stg,
    opennsl_vlan_t vid);

int __wrap_opennsl_port_dtag_mode_get(int unit, opennsl_port_t port, int* mode);

int __wrap_opennsl_stat_multi_get(
    int unit,
    opennsl_port_t port,
    int nstat,
    opennsl_stat_val_t* stat_arr,
    uint64* value_arr);

int __wrap_opennsl_l3_info(int unit, opennsl_l3_info_t* l3info);

int __wrap_opennsl_knet_netif_destroy(int unit, int netif_id);

int __wrap_opennsl_l2_addr_delete(
    int unit,
    opennsl_mac_t mac,
    opennsl_vlan_t vid);

int __wrap_opennsl_vlan_control_port_set(
    int unit,
    int port,
    opennsl_vlan_control_port_t type,
    int arg);

int __wrap_opennsl_info_get(int unit, opennsl_info_t* info);

int __wrap_opennsl_cosq_bst_profile_get(
    int unit,
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosq,
    opennsl_bst_stat_id_t bid,
    opennsl_cosq_bst_profile_t* profile);

int __wrap_opennsl_port_interface_set(
    int unit,
    opennsl_port_t port,
    opennsl_port_if_t intf);

int __wrap_opennsl_vlan_default_get(int unit, opennsl_vlan_t* vid_ptr);

int __wrap_opennsl_linkscan_mode_set(int unit, opennsl_port_t port, int mode);

int __wrap_opennsl_knet_filter_traverse(
    int unit,
    opennsl_knet_filter_traverse_cb trav_fn,
    void* user_data);

int __wrap_opennsl_port_speed_set(int unit, opennsl_port_t port, int speed);

int __wrap_opennsl_stg_stp_set(
    int unit,
    opennsl_stg_t stg,
    opennsl_port_t port,
    int stp_state);

int __wrap_opennsl_l3_route_add(int unit, opennsl_l3_route_t* info);

int __wrap_opennsl_vlan_default_set(int unit, opennsl_vlan_t vid);

int __wrap_opennsl_port_speed_get(int unit, opennsl_port_t port, int* speed);

int __wrap_opennsl_port_learn_get(int unit, opennsl_port_t port, uint32* flags);

int __wrap_opennsl_stat_clear(int unit, opennsl_port_t port);

int __wrap_opennsl_l3_egress_multipath_delete(
    int unit,
    opennsl_if_t mpintf,
    opennsl_if_t intf);

int __wrap_opennsl_l2_station_delete(int unit, int station_id);

int __wrap_opennsl_attach(int unit, char* type, char* subtype, int remunit);

int __wrap_opennsl_l2_age_timer_set(int unit, int age_seconds);

int __wrap_opennsl_port_selective_get(
    int unit,
    opennsl_port_t port,
    opennsl_port_info_t* info);

char* __wrap_opennsl_port_name(int unit, int port);

int __wrap_opennsl_l3_route_multipath_get(
    int unit,
    opennsl_l3_route_t* the_route,
    opennsl_l3_route_t* path_array,
    int max_path,
    int* path_count);

int __wrap_opennsl_stg_list(int unit, opennsl_stg_t** list, int* count);

int __wrap_opennsl_cosq_bst_stat_clear(
    int unit,
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosq,
    opennsl_bst_stat_id_t bid);

int __wrap_opennsl_attach_check(int unit);

int __wrap_opennsl_port_ability_local_get(
    int unit,
    opennsl_port_t port,
    opennsl_port_ability_t* local_ability_mask);

int __wrap_opennsl_vlan_gport_delete_all(int unit, opennsl_vlan_t vlan);

int __wrap_opennsl_stg_destroy(int unit, opennsl_stg_t stg);

int __wrap_opennsl_stg_default_get(int unit, opennsl_stg_t* stg_ptr);

int __wrap_opennsl_l2_addr_get(
    int unit,
    opennsl_mac_t mac_addr,
    opennsl_vlan_t vid,
    opennsl_l2_addr_t* l2addr);

int __wrap_opennsl_port_interface_get(
    int unit,
    opennsl_port_t port,
    opennsl_port_if_t* intf);

int __wrap_opennsl_stg_stp_get(
    int unit,
    opennsl_stg_t stg,
    opennsl_port_t port,
    int* stp_state);

int __wrap_opennsl_l3_route_traverse(
    int unit,
    uint32 flags,
    uint32 start,
    uint32 end,
    opennsl_l3_route_traverse_cb trav_fn,
    void* user_data);

int __wrap_opennsl_port_selective_set(
    int unit,
    opennsl_port_t port,
    opennsl_port_info_t* info);

int __wrap_opennsl_port_frame_max_get(int unit, opennsl_port_t port, int* size);

void __wrap_opennsl_l3_info_t_init(opennsl_l3_info_t* info);

int __wrap_opennsl_l3_egress_ecmp_create(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp,
    int intf_count,
    opennsl_if_t* intf_array);

int __wrap_opennsl_attach_max(int* max_units);

int __wrap_opennsl_l3_egress_ecmp_add(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp,
    opennsl_if_t intf);

int __wrap_opennsl_rx_stop(int unit, opennsl_rx_cfg_t* cfg);

int __wrap_opennsl_l3_route_max_ecmp_set(int unit, int max);

int __wrap_opennsl_vlan_port_remove(
    int unit,
    opennsl_vlan_t vid,
    opennsl_pbmp_t pbmp);

int __wrap_opennsl_knet_filter_destroy(int unit, int filter_id);

}
