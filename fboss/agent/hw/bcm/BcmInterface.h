#pragma once

#include "fboss/agent/hw/bcm/BcmSdkVer.h"

extern "C" {
#if (defined(IS_OPENNSA))
#define BCM_WARM_BOOT_SUPPORT
#endif

#include <bcm/cosq.h>
#include <bcm/error.h>
#include <bcm/init.h>
#include <bcm/knet.h>
#include <bcm/l2.h>
#include <bcm/l3.h>
#include <bcm/link.h>
#include <bcm/mpls.h>
#include <bcm/pkt.h>
#include <bcm/port.h>
#include <bcm/rx.h>
#include <bcm/stack.h>
#include <bcm/stat.h>
#include <bcm/stg.h>
#include <bcm/switch.h>
#include <bcm/trunk.h>
#include <bcm/tx.h>
#include <bcm/types.h>
#include <bcm/vlan.h>

#include <bcm/pktio.h>
#include <bcm/pktio_defs.h>
}

namespace facebook::fboss {

class BcmInterface {
 public:
  virtual ~BcmInterface() = default;

  virtual int
  bcm_port_gport_get(int unit, bcm_port_t port, bcm_gport_t* gport) = 0;

  virtual int
  bcm_port_vlan_member_set(int unit, bcm_port_t port, uint32 flags) = 0;

  virtual int bcm_l2_age_timer_set(int unit, int age_seconds) = 0;

  virtual int bcm_stg_destroy(int unit, bcm_stg_t stg) = 0;

  virtual int bcm_l3_intf_find_vlan(int unit, bcm_l3_intf_t* intf) = 0;

  virtual int bcm_knet_netif_create(int unit, bcm_knet_netif_t* netif) = 0;

  virtual int
  bcm_l2_traverse(int unit, bcm_l2_traverse_cb trav_fn, void* user_data) = 0;

  virtual int bcm_l3_route_delete_by_interface(
      int unit,
      bcm_l3_route_t* info) = 0;

  virtual int bcm_l3_host_delete_by_interface(
      int unit,
      bcm_l3_host_t* info) = 0;

  virtual int bcm_l3_egress_ecmp_traverse(
      int unit,
      bcm_l3_egress_ecmp_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int bcm_l2_age_timer_get(int unit, int* age_seconds) = 0;

  virtual int bcm_vlan_list(int unit, bcm_vlan_data_t** listp, int* countp) = 0;

  virtual int bcm_vlan_destroy_all(int unit) = 0;

  virtual int bcm_l3_ecmp_member_add(
      int unit,
      bcm_if_t ecmp_group_id,
      bcm_l3_ecmp_member_t* ecmp_member) = 0;

  virtual int bcm_l3_egress_ecmp_add(
      int unit,
      bcm_l3_egress_ecmp_t* ecmp,
      bcm_if_t intf) = 0;

  virtual int
  bcm_rx_unregister(int unit, bcm_rx_cb_f callback, uint8 priority) = 0;

  virtual int bcm_switch_control_port_get(
      int unit,
      bcm_port_t port,
      bcm_switch_control_t type,
      int* arg) = 0;

  virtual int bcm_info_get(int unit, bcm_info_t* info) = 0;

  virtual int
  bcm_port_untagged_vlan_set(int unit, bcm_port_t port, bcm_vlan_t vid) = 0;

  virtual int bcm_rx_control_set(int unit, bcm_rx_control_t type, int arg) = 0;

  virtual int bcm_knet_netif_destroy(int unit, int netif_id) = 0;

  virtual void bcm_port_config_t_init(bcm_port_config_t* config) = 0;

  virtual int bcm_port_config_get(int unit, bcm_port_config_t* config) = 0;

  virtual int bcm_vlan_destroy(int unit, bcm_vlan_t vid) = 0;

  virtual int bcm_stg_create(int unit, bcm_stg_t* stg_ptr) = 0;

  virtual int bcm_l3_ecmp_get(
      int unit,
      bcm_l3_egress_ecmp_t* ecmp_info,
      int ecmp_member_size,
      bcm_l3_ecmp_member_t* ecmp_member_array,
      int* ecmp_member_count) = 0;

  virtual int bcm_l3_egress_ecmp_get(
      int unit,
      bcm_l3_egress_ecmp_t* ecmp,
      int intf_size,
      bcm_if_t* intf_array,
      int* intf_count) = 0;
  virtual int bcm_linkscan_mode_set(int unit, bcm_port_t port, int mode) = 0;

  virtual int bcm_l3_host_delete(int unit, bcm_l3_host_t* ip_addr) = 0;

  virtual int bcm_l3_intf_create(int unit, bcm_l3_intf_t* intf) = 0;

  virtual int bcm_l3_route_delete(int unit, bcm_l3_route_t* info) = 0;

  virtual int bcm_cosq_bst_stat_sync(int unit, bcm_bst_stat_id_t bid) = 0;

  virtual void bcm_knet_filter_t_init(bcm_knet_filter_t* filter) = 0;

  virtual int bcm_l3_host_find(int unit, bcm_l3_host_t* info) = 0;

  virtual int bcm_stg_default_set(int unit, bcm_stg_t stg) = 0;

  virtual int bcm_port_control_get(
      int unit,
      bcm_port_t port,
      bcm_port_control_t type,
      int* value) = 0;

  virtual int bcm_l3_egress_multipath_find(
      int unit,
      int intf_count,
      bcm_if_t* intf_array,
      bcm_if_t* mpintf) = 0;

  virtual int bcm_switch_control_port_set(
      int unit,
      bcm_port_t port,
      bcm_switch_control_t type,
      int arg) = 0;

  virtual int bcm_l3_egress_ecmp_ethertype_set(
      int unit,
      uint32 flags,
      int ethertype_count,
      int* ethertype_array) = 0;

  virtual int
  bcm_l3_egress_ecmp_member_status_set(int unit, bcm_if_t intf, int status) = 0;

  virtual int bcm_l3_egress_ecmp_member_status_get(
      int unit,
      bcm_if_t intf,
      int* status) = 0;

  virtual int
  bcm_vlan_list_destroy(int unit, bcm_vlan_data_t* list, int count) = 0;

  virtual int
  bcm_vlan_port_remove(int unit, bcm_vlan_t vid, bcm_pbmp_t pbmp) = 0;

  virtual int bcm_l3_route_traverse(
      int unit,
      uint32 flags,
      uint32 start,
      uint32 end,
      bcm_l3_route_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int bcm_l2_station_delete(int unit, int station_id) = 0;

  virtual int bcm_linkscan_mode_get(int unit, bcm_port_t port, int* mode) = 0;

  virtual int bcm_switch_pkt_trace_info_get(
      int unit,
      uint32 options,
      uint8 port,
      int len,
      uint8* data,
      bcm_switch_pkt_trace_info_t* pkt_trace_info) = 0;

  virtual bcm_ip_t bcm_ip_mask_create(int len) = 0;

  virtual int bcm_l3_egress_multipath_get(
      int unit,
      bcm_if_t mpintf,
      int intf_size,
      bcm_if_t* intf_array,
      int* intf_count) = 0;

  virtual int bcm_stg_default_get(int unit, bcm_stg_t* stg_ptr) = 0;

  virtual int bcm_linkscan_enable_get(int unit, int* us) = 0;

  virtual int
  bcm_stg_stp_get(int unit, bcm_stg_t stg, bcm_port_t port, int* stp_state) = 0;

  virtual int bcm_attach_max(int* max_units) = 0;

  virtual int
  bcm_port_local_get(int unit, bcm_gport_t gport, bcm_port_t* local_port) = 0;

  virtual void bcm_l3_egress_ecmp_t_init(bcm_l3_egress_ecmp_t* ecmp) = 0;

  virtual int bcm_cosq_bst_profile_set(
      int unit,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq,
      bcm_bst_stat_id_t bid,
      bcm_cosq_bst_profile_t* profile) = 0;

  virtual int
  bcm_cosq_mapping_get(int unit, bcm_cos_t priority, bcm_cos_queue_t* cosq) = 0;

  virtual int bcm_port_control_set(
      int unit,
      bcm_port_t port,
      bcm_port_control_t type,
      int value) = 0;

  virtual int bcm_tx(int unit, bcm_pkt_t* tx_pkt, void* cookie) = 0;

  virtual int bcm_pktio_tx(int unit, bcm_pktio_pkt_t* tx_pkt) = 0;

  virtual int
  bcm_l3_egress_get(int unit, bcm_if_t intf, bcm_l3_egress_t* egr) = 0;

  virtual void bcm_l3_egress_t_init(bcm_l3_egress_t* egr) = 0;

  virtual int
  bcm_port_stat_enable_set(int unit, bcm_gport_t port, int enable) = 0;

#if (defined(IS_OPENNSA) || defined(BCM_SDK_VERSION_GTE_6_5_21))
  virtual int bcm_port_fdr_config_set(
      int unit,
      bcm_port_t port,
      bcm_port_fdr_config_t* fdr_config) = 0;

  virtual int bcm_port_fdr_config_get(
      int unit,
      bcm_port_t port,
      bcm_port_fdr_config_t* fdr_config) = 0;

  virtual int bcm_port_fdr_stats_get(
      int unit,
      bcm_port_t port,
      bcm_port_fdr_stats_t* fdr_stats) = 0;
#endif

  virtual int bcm_vlan_default_get(int unit, bcm_vlan_t* vid_ptr) = 0;

  virtual int bcm_l3_init(int unit) = 0;

  virtual int
  bcm_l2_station_get(int unit, int station_id, bcm_l2_station_t* station) = 0;

  virtual int bcm_linkscan_detach(int unit) = 0;

  virtual int bcm_rx_stop(int unit, bcm_rx_cfg_t* cfg) = 0;

  virtual int bcm_l3_egress_multipath_create(
      int unit,
      uint32 flags,
      int intf_count,
      bcm_if_t* intf_array,
      bcm_if_t* mpintf) = 0;

  virtual int bcm_port_ability_advert_get(
      int unit,
      bcm_port_t port,
      bcm_port_ability_t* ability_mask) = 0;

  virtual int bcm_l3_route_multipath_get(
      int unit,
      bcm_l3_route_t* the_route,
      bcm_l3_route_t* path_array,
      int max_path,
      int* path_count) = 0;

  virtual int
  bcm_port_queued_count_get(int unit, bcm_port_t port, uint32* count) = 0;

  virtual int bcm_l3_info(int unit, bcm_l3_info_t* l3info) = 0;

  virtual int bcm_port_learn_get(int unit, bcm_port_t port, uint32* flags) = 0;

  virtual int bcm_port_frame_max_set(int unit, bcm_port_t port, int size) = 0;

  virtual int bcm_l3_route_delete_all(int unit, bcm_l3_route_t* info) = 0;

  virtual int bcm_rx_cfg_get(int unit, bcm_rx_cfg_t* cfg) = 0;

  virtual int bcm_port_ability_advert_set(
      int unit,
      bcm_port_t port,
      bcm_port_ability_t* ability_mask) = 0;

  virtual int bcm_knet_filter_destroy(int unit, int filter_id) = 0;

  virtual int
  bcm_l3_egress_find(int unit, bcm_l3_egress_t* egr, bcm_if_t* intf) = 0;

  virtual int bcm_l3_route_get(int unit, bcm_l3_route_t* info) = 0;

  virtual int bcm_cosq_bst_stat_get(
      int unit,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq,
      bcm_bst_stat_id_t bid,
      uint32 options,
      uint64* value) = 0;

  virtual int bcm_cosq_bst_stat_extended_get(
      int unit,
      bcm_cosq_object_id_t* id,
      bcm_bst_stat_id_t bid,
      uint32 options,
      uint64* value) = 0;

  virtual int bcm_udf_hash_config_add(
      int unit,
      uint32 options,
      bcm_udf_hash_config_t* config) = 0;

  virtual int bcm_udf_hash_config_delete(
      int unit,
      bcm_udf_hash_config_t* config) = 0;

  virtual int bcm_udf_create(
      int unit,
      bcm_udf_alloc_hints_t* hints,
      bcm_udf_t* udf_info,
      bcm_udf_id_t* udf_id) = 0;

  virtual int bcm_udf_destroy(int unit, bcm_udf_id_t udf_id) = 0;

  virtual int bcm_udf_pkt_format_create(
      int unit,
      bcm_udf_pkt_format_options_t options,
      bcm_udf_pkt_format_info_t* pkt_format,
      bcm_udf_pkt_format_id_t* pkt_format_id) = 0;

  virtual int bcm_udf_pkt_format_destroy(
      int unit,
      bcm_udf_pkt_format_id_t pkt_format_id) = 0;

  virtual int bcm_udf_pkt_format_add(
      int unit,
      bcm_udf_id_t udf_id,
      bcm_udf_pkt_format_id_t pkt_format_id) = 0;

  virtual int
  bcm_port_pause_addr_set(int unit, bcm_port_t port, bcm_mac_t mac) = 0;

  virtual int bcm_udf_pkt_format_delete(
      int unit,
      bcm_udf_id_t udf_id,
      bcm_udf_pkt_format_id_t pkt_format_id) = 0;

  virtual int bcm_udf_pkt_format_get(
      int unit,
      bcm_udf_pkt_format_id_t pkt_format_id,
      int max,
      bcm_udf_id_t* udf_id_list,
      int* actual) = 0;

  virtual int bcm_udf_hash_config_get(
      int unit,
      bcm_udf_hash_config_t* config) = 0;

  virtual int bcm_udf_pkt_format_info_get(
      int unit,
      bcm_udf_pkt_format_id_t pkt_format_id,
      bcm_udf_pkt_format_info_t* pkt_format) = 0;

  virtual void bcm_udf_pkt_format_info_t_init(
      bcm_udf_pkt_format_info_t* pkt_format) = 0;

  virtual void bcm_udf_alloc_hints_t_init(bcm_udf_alloc_hints_t* udf_hints) = 0;

  virtual void bcm_udf_t_init(bcm_udf_t* udf_info) = 0;

  virtual void bcm_udf_hash_config_t_init(bcm_udf_hash_config_t* config) = 0;

  virtual int bcm_udf_init(int unit) = 0;

  virtual int
  bcm_udf_get(int unit, bcm_udf_id_t udf_id, bcm_udf_t* udf_info) = 0;

  virtual int bcm_stat_clear(int unit, bcm_port_t port) = 0;

  virtual int bcm_l3_route_max_ecmp_set(int unit, int max) = 0;

  virtual int bcm_switch_event_unregister(
      int unit,
      bcm_switch_event_cb_t cb,
      void* userdata) = 0;

  virtual int bcm_attach(int unit, char* type, char* subtype, int remunit) = 0;

  virtual int bcm_port_speed_set(int unit, bcm_port_t port, int speed) = 0;

  virtual int bcm_l3_egress_destroy(int unit, bcm_if_t intf) = 0;

  virtual int bcm_port_enable_set(int unit, bcm_port_t port, int enable) = 0;

  virtual int bcm_l2_addr_delete(int unit, bcm_mac_t mac, bcm_vlan_t vid) = 0;

  virtual int bcm_vlan_port_add(
      int unit,
      bcm_vlan_t vid,
      bcm_pbmp_t pbmp,
      bcm_pbmp_t ubmp) = 0;

  virtual int
  bcm_port_selective_get(int unit, bcm_port_t port, bcm_port_info_t* info) = 0;

  virtual int bcm_cosq_bst_profile_get(
      int unit,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq,
      bcm_bst_stat_id_t bid,
      bcm_cosq_bst_profile_t* profile) = 0;

  virtual int bcm_l3_ecmp_member_delete(
      int unit,
      bcm_if_t ecmp_group_id,
      bcm_l3_ecmp_member_t* ecmp_member) = 0;

  virtual int bcm_l3_egress_ecmp_delete(
      int unit,
      bcm_l3_egress_ecmp_t* ecmp,
      bcm_if_t intf) = 0;

  virtual int
  bcm_port_link_status_get(int unit, bcm_port_t port, int* status) = 0;

  virtual char* bcm_port_name(int unit, int port) = 0;

  virtual int bcm_stg_list_destroy(int unit, bcm_stg_t* list, int count) = 0;

  virtual int bcm_linkscan_enable_set(int unit, int us) = 0;

  virtual int bcm_port_dtag_mode_set(int unit, bcm_port_t port, int mode) = 0;

  virtual int bcm_l3_ecmp_create(
      int unit,
      uint32 options,
      bcm_l3_egress_ecmp_t* ecmp_info,
      int ecmp_member_count,
      bcm_l3_ecmp_member_t* ecmp_member_array) = 0;

  virtual int bcm_l3_egress_ecmp_create(
      int unit,
      bcm_l3_egress_ecmp_t* ecmp,
      int intf_count,
      bcm_if_t* intf_array) = 0;

  virtual int bcm_stk_my_modid_get(int unit, int* my_modid) = 0;

  virtual int bcm_l3_intf_delete(int unit, bcm_l3_intf_t* intf) = 0;

  virtual int
  bcm_l3_egress_multipath_add(int unit, bcm_if_t mpintf, bcm_if_t intf) = 0;

  virtual int bcm_l3_egress_traverse(
      int unit,
      bcm_l3_egress_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int bcm_knet_init(int unit) = 0;

  virtual int bcm_l3_route_add(int unit, bcm_l3_route_t* info) = 0;

  virtual int bcm_stat_multi_get(
      int unit,
      bcm_port_t port,
      int nstat,
      bcm_stat_val_t* stat_arr,
      uint64* value_arr) = 0;

  virtual int bcm_l3_intf_get(int unit, bcm_l3_intf_t* intf) = 0;

  virtual int bcm_detach(int unit) = 0;

  virtual int _bcm_shutdown(int unit) = 0;

  virtual int soc_shutdown(int unit) = 0;

  virtual int bcm_l2_addr_get(
      int unit,
      bcm_mac_t mac_addr,
      bcm_vlan_t vid,
      bcm_l2_addr_t* l2addr) = 0;

  virtual int bcm_stat_get(
      int unit,
      bcm_port_t port,
      bcm_stat_val_t type,
      uint64* value) = 0;

  virtual int bcm_stat_sync_multi_get(
      int unit,
      bcm_port_t port,
      int nstat,
      bcm_stat_val_t* stat_arr,
      uint64* value_arr) = 0;

  virtual int
  bcm_stg_stp_set(int unit, bcm_stg_t stg, bcm_port_t port, int stp_state) = 0;

  virtual int bcm_port_untagged_vlan_get(
      int unit,
      bcm_port_t port,
      bcm_vlan_t* vid_ptr) = 0;

  virtual int bcm_vlan_default_set(int unit, bcm_vlan_t vid) = 0;

  virtual int bcm_linkscan_unregister(int unit, bcm_linkscan_handler_t f) = 0;

  virtual int bcm_l3_ecmp_destroy(int unit, bcm_if_t ecmp_group_id) = 0;

  virtual int bcm_l3_egress_ecmp_destroy(
      int unit,
      bcm_l3_egress_ecmp_t* ecmp) = 0;

  virtual int bcm_ip6_mask_create(bcm_ip6_t ip6, int len) = 0;

  virtual int bcm_rx_start(int unit, bcm_rx_cfg_t* cfg) = 0;

  virtual int bcm_l3_egress_create(
      int unit,
      uint32 flags,
      bcm_l3_egress_t* egr,
      bcm_if_t* if_id) = 0;

  virtual int bcm_l3_intf_find(int unit, bcm_l3_intf_t* intf) = 0;

  virtual int bcm_stg_list(int unit, bcm_stg_t** list, int* count) = 0;

  virtual int
  bcm_port_vlan_member_get(int unit, bcm_port_t port, uint32* flags) = 0;

  virtual int bcm_stg_vlan_add(int unit, bcm_stg_t stg, bcm_vlan_t vid) = 0;

  virtual void bcm_knet_netif_t_init(bcm_knet_netif_t* netif) = 0;

  virtual int bcm_rx_control_get(int unit, bcm_rx_control_t type, int* arg) = 0;

  virtual int
  bcm_pkt_flags_init(int unit, bcm_pkt_t* pkt, uint32 init_flags) = 0;

  virtual int bcm_l3_egress_multipath_traverse(
      int unit,
      bcm_l3_egress_multipath_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int bcm_l3_host_traverse(
      int unit,
      uint32 flags,
      uint32 start,
      uint32 end,
      bcm_l3_host_traverse_cb cb,
      void* user_data) = 0;

  virtual int bcm_l3_host_add(int unit, bcm_l3_host_t* info) = 0;

  virtual int bcm_port_learn_set(int unit, bcm_port_t port, uint32 flags) = 0;

  virtual int
  bcm_l2_station_add(int unit, int* station_id, bcm_l2_station_t* station) = 0;

  virtual int bcm_port_phy_modify(
      int unit,
      bcm_port_t port,
      uint32 flags,
      uint32 phy_reg_addr,
      uint32 phy_data,
      uint32 phy_mask) = 0;

  virtual int bcm_l3_host_delete_all(int unit, bcm_l3_host_t* info) = 0;

  virtual int bcm_l2_addr_add(int unit, bcm_l2_addr_t* l2addr) = 0;

  virtual int bcm_cosq_bst_stat_multi_get(
      int unit,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq,
      uint32 options,
      int max_values,
      bcm_bst_stat_id_t* id_list,
      uint64* values) = 0;

  virtual int bcm_l3_egress_ecmp_find(
      int unit,
      int intf_count,
      bcm_if_t* intf_array,
      bcm_l3_egress_ecmp_t* ecmp) = 0;

  virtual int bcm_pkt_free(int unit, bcm_pkt_t* pkt) = 0;

  virtual int bcm_vlan_control_port_set(
      int unit,
      int port,
      bcm_vlan_control_port_t type,
      int arg) = 0;

  virtual int bcm_vlan_create(int unit, bcm_vlan_t vid) = 0;

  virtual int bcm_attach_check(int unit) = 0;

  virtual int bcm_port_dtag_mode_get(int unit, bcm_port_t port, int* mode) = 0;

  virtual int
  bcm_pkt_alloc(int unit, int size, uint32 flags, bcm_pkt_t** pkt_buf) = 0;

  virtual int bcm_knet_filter_traverse(
      int unit,
      bcm_knet_filter_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int bcm_port_speed_max(int unit, bcm_port_t port, int* speed) = 0;

  virtual int bcm_linkscan_mode_set_pbm(int unit, bcm_pbmp_t pbm, int mode) = 0;

  virtual int bcm_port_ability_local_get(
      int unit,
      bcm_port_t port,
      bcm_port_ability_t* local_ability_mask) = 0;

  virtual int bcm_port_enable_get(int unit, bcm_port_t port, int* enable) = 0;

  virtual int bcm_port_frame_max_get(int unit, bcm_port_t port, int* size) = 0;

  virtual int bcm_cosq_bst_stat_clear(
      int unit,
      bcm_gport_t gport,
      bcm_cos_queue_t cosq,
      bcm_bst_stat_id_t bid) = 0;

  virtual int
  bcm_cosq_mapping_set(int unit, bcm_cos_t priority, bcm_cos_queue_t cosq) = 0;

  virtual int
  bcm_port_selective_set(int unit, bcm_port_t port, bcm_port_info_t* info) = 0;

  virtual int
  bcm_port_interface_set(int unit, bcm_port_t port, bcm_port_if_t intf) = 0;

  virtual int bcm_l3_route_max_ecmp_get(int unit, int* max) = 0;

  virtual int
  bcm_port_interface_get(int unit, bcm_port_t port, bcm_port_if_t* intf) = 0;

  virtual int bcm_l3_egress_multipath_destroy(int unit, bcm_if_t mpintf) = 0;

  virtual int bcm_vlan_gport_delete_all(int unit, bcm_vlan_t vlan) = 0;

  virtual int
  bcm_l3_egress_multipath_delete(int unit, bcm_if_t mpintf, bcm_if_t intf) = 0;

  virtual int bcm_knet_filter_create(int unit, bcm_knet_filter_t* filter) = 0;

  virtual int
  bcm_switch_control_set(int unit, bcm_switch_control_t type, int arg) = 0;

  virtual int bcm_rx_free(int unit, void* pkt_data) = 0;

  virtual int bcm_rx_register(
      int unit,
      const char* name,
      bcm_rx_cb_f callback,
      uint8 priority,
      void* cookie,
      uint32 flags) = 0;

  virtual int bcm_port_speed_get(int unit, bcm_port_t port, int* speed) = 0;

  virtual int bcm_knet_netif_traverse(
      int unit,
      bcm_knet_netif_traverse_cb trav_fn,
      void* user_data) = 0;

  virtual int bcm_switch_event_register(
      int unit,
      bcm_switch_event_cb_t cb,
      void* userdata) = 0;

  virtual int bcm_linkscan_register(int unit, bcm_linkscan_handler_t f) = 0;

  virtual int
  bcm_switch_control_get(int unit, bcm_switch_control_t type, int* arg) = 0;

  virtual int bcm_trunk_init(int unit) = 0;

  virtual int bcm_trunk_get(
      int unit,
      bcm_trunk_t tid,
      bcm_trunk_info_t* t_data,
      int member_max,
      bcm_trunk_member_t* member_array,
      int* member_count) = 0;

  virtual int bcm_trunk_find(
      int unit,
      bcm_module_t modid,
      bcm_port_t port,
      bcm_trunk_t* trunk) = 0;

  virtual int bcm_trunk_destroy(int unit, bcm_trunk_t tid) = 0;

  virtual int bcm_trunk_create(int unit, uint32 flags, bcm_trunk_t* tid) = 0;

  virtual int bcm_trunk_set(
      int unit,
      bcm_trunk_t tid,
      bcm_trunk_info_t* trunk_info,
      int member_count,
      bcm_trunk_member_t* member_array) = 0;

  virtual int bcm_trunk_member_add(
      int unit,
      bcm_trunk_t tid,
      bcm_trunk_member_t* member) = 0;

  virtual int bcm_trunk_member_delete(
      int unit,
      bcm_trunk_t tid,
      bcm_trunk_member_t* member) = 0;

  virtual int bcm_port_ifg_get(
      int unit,
      bcm_port_t port,
      int speed,
      bcm_port_duplex_t duplex,
      int* bit_times) = 0;

  virtual int bcm_port_ifg_set(
      int unit,
      bcm_port_t port,
      int speed,
      bcm_port_duplex_t duplex,
      int bit_times) = 0;
};

} // namespace facebook::fboss
