#include "OpenNSLMock.h"

#include <folly/ThreadLocal.h>

using namespace facebook::fboss;

//
// Singleton instance of the mock sdk
//
namespace {
folly::ThreadLocalPtr<OpenNSLMock> sdkMock;
}

namespace facebook { namespace fboss {

void
OpenNSLMock::resetMock(std::unique_ptr<OpenNSLMock> mock) noexcept {
  sdkMock.reset(std::move(mock));
}

OpenNSLMock* OpenNSLMock::getMock() noexcept {
  return sdkMock.get();
}

void OpenNSLMock::destroyMock() noexcept {
  delete sdkMock.release();
}

}}

extern "C" {

int __wrap_opennsl_vlan_control_port_set(
    int unit,
    int port,
    opennsl_vlan_control_port_t type,
    int arg) {
   return sdkMock.get()->opennsl_vlan_control_port_set(unit, port, type, arg);
}

int __wrap_opennsl_cosq_bst_stat_sync(int unit, opennsl_bst_stat_id_t bid) {
   return sdkMock.get()->opennsl_cosq_bst_stat_sync(unit, bid);
}

int __wrap_opennsl_stat_get(
    int unit,
    opennsl_port_t port,
    opennsl_stat_val_t type,
    uint64* value) {
   return sdkMock.get()->opennsl_stat_get(unit, port, type, value);
}

int __wrap_opennsl_linkscan_register(int unit, opennsl_linkscan_handler_t f) {
   return sdkMock.get()->opennsl_linkscan_register(unit, f);
}

int __wrap_opennsl_l3_egress_ecmp_delete(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp,
    opennsl_if_t intf) {
   return sdkMock.get()->opennsl_l3_egress_ecmp_delete(unit, ecmp, intf);
}

int __wrap_opennsl_rx_register(
    int unit,
    const char* name,
    opennsl_rx_cb_f callback,
    uint8 priority,
    void* cookie,
    uint32 flags) {
   return sdkMock.get()->opennsl_rx_register(
      unit, name, callback, priority, cookie, flags);
}

int __wrap_opennsl_port_speed_get(int unit, opennsl_port_t port, int* speed) {
   return sdkMock.get()->opennsl_port_speed_get(unit, port, speed);
}

int __wrap_opennsl_stk_my_modid_get(int unit, int* my_modid) {
   return sdkMock.get()->opennsl_stk_my_modid_get(unit, my_modid);
}

int __wrap_opennsl_stg_list_destroy(int unit, opennsl_stg_t* list, int count) {
   return sdkMock.get()->opennsl_stg_list_destroy(unit, list, count);
}

int __wrap_opennsl_trunk_member_delete(
    int unit,
    opennsl_trunk_t tid,
    opennsl_trunk_member_t* member) {
   return sdkMock.get()->opennsl_trunk_member_delete(unit, tid, member);
}

int __wrap_opennsl_trunk_destroy(int unit, opennsl_trunk_t tid) {
   return sdkMock.get()->opennsl_trunk_destroy(unit, tid);
}

int __wrap_opennsl_trunk_detach(int unit) {
  return sdkMock.get()->opennsl_trunk_detach(unit);
}

void __wrap_opennsl_trunk_member_t_init(opennsl_trunk_member_t* trunk_member) {
  sdkMock.get()->opennsl_trunk_member_t_init(trunk_member);
}

int __wrap_opennsl_trunk_set(
    int unit,
    opennsl_trunk_t tid,
    opennsl_trunk_info_t* trunk_info,
    int member_count,
    opennsl_trunk_member_t* member_array) {
  return sdkMock.get()->opennsl_trunk_set(
      unit, tid, trunk_info, member_count, member_array);
}

int __wrap_opennsl_trunk_chip_info_get(
    int unit,
    opennsl_trunk_chip_info_t* ta_info) {
  return sdkMock.get()->opennsl_trunk_chip_info_get(unit, ta_info);
}

int __wrap_opennsl_trunk_get(
    int unit,
    opennsl_trunk_t tid,
    opennsl_trunk_info_t* t_data,
    int member_max,
    opennsl_trunk_member_t* member_array,
    int* member_count) {
  return sdkMock.get()->opennsl_trunk_get(
      unit, tid, t_data, member_max, member_array, member_count);
}

void __wrap_opennsl_trunk_info_t_init(opennsl_trunk_info_t* trunk_info) {
  sdkMock.get()->opennsl_trunk_info_t_init(trunk_info);
}

int __wrap_opennsl_trunk_member_add(
    int unit,
    opennsl_trunk_t tid,
    opennsl_trunk_member_t* member) {
  return sdkMock.get()->opennsl_trunk_member_add(unit, tid, member);
}

int __wrap_opennsl_trunk_init(int unit) {
  return sdkMock.get()->opennsl_trunk_init(unit);
}

int __wrap_opennsl_trunk_create(int unit, uint32 flags, opennsl_trunk_t* tid) {
  return sdkMock.get()->opennsl_trunk_create(unit, flags, tid);
}

int __wrap_opennsl_trunk_find(
    int unit,
    opennsl_module_t modid,
    opennsl_port_t port,
    opennsl_trunk_t* tid) {
  return sdkMock.get()->opennsl_trunk_find(unit, modid, port, tid);
}

int __wrap_opennsl_trunk_psc_set(int unit, opennsl_trunk_t tid, int psc) {
  return sdkMock.get()->opennsl_trunk_psc_set(unit, tid, psc);
}

int __wrap_opennsl_rx_control_set(
    int unit,
    opennsl_rx_control_t type,
    int arg) {
   return sdkMock.get()->opennsl_rx_control_set(unit, type, arg);
}

int __wrap_opennsl_info_get(int unit, opennsl_info_t* info) {
   return sdkMock.get()->opennsl_info_get(unit, info);
}

int __wrap_opennsl_stg_list(int unit, opennsl_stg_t** list, int* count) {
   return sdkMock.get()->opennsl_stg_list(unit, list, count);
}

int __wrap_opennsl_l3_init(int unit) {
   return sdkMock.get()->opennsl_l3_init(unit);
}

opennsl_ip_t __wrap_opennsl_ip_mask_create(int len) {
   return sdkMock.get()->opennsl_ip_mask_create(len);
}

int __wrap_opennsl_l3_egress_find(
    int unit,
    opennsl_l3_egress_t* egr,
    opennsl_if_t* intf) {
   return sdkMock.get()->opennsl_l3_egress_find(unit, egr, intf);
}

int __wrap_opennsl_port_dtag_mode_get(
    int unit,
    opennsl_port_t port,
    int* mode) {
   return sdkMock.get()->opennsl_port_dtag_mode_get(unit, port, mode);
}

int __wrap_opennsl_port_ability_advert_get(
    int unit,
    opennsl_port_t port,
    opennsl_port_ability_t* ability_mask) {
  return sdkMock.get()->opennsl_port_ability_advert_get(
      unit, port, ability_mask);
}

int __wrap_opennsl_port_config_get(int unit, opennsl_port_config_t* config) {
  return sdkMock.get()->opennsl_port_config_get(unit, config);
}

int __wrap_opennsl_port_gport_get(
    int unit,
    opennsl_port_t port,
    opennsl_gport_t* gport) {
  return sdkMock.get()->opennsl_port_gport_get(unit, port, gport);
}

int __wrap_opennsl_l3_egress_ecmp_add(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp,
    opennsl_if_t intf) {
  return sdkMock.get()->opennsl_l3_egress_ecmp_add(unit, ecmp, intf);
}

int __wrap_opennsl_l3_route_delete_all(int unit, opennsl_l3_route_t* info) {
  return sdkMock.get()->opennsl_l3_route_delete_all(unit, info);
}

int __wrap_opennsl_port_learn_get(
    int unit,
    opennsl_port_t port,
    uint32* flags) {
  return sdkMock.get()->opennsl_port_learn_get(unit, port, flags);
}

int __wrap_opennsl_port_queued_count_get(
    int unit,
    opennsl_port_t port,
    uint32* count) {
  return sdkMock.get()->opennsl_port_queued_count_get(unit, port, count);
}

int __wrap_opennsl_port_selective_set(
    int unit,
    opennsl_port_t port,
    opennsl_port_info_t* info) {
  return sdkMock.get()->opennsl_port_selective_set(unit, port, info);
}

int __wrap_opennsl_rx_start(int unit, opennsl_rx_cfg_t* cfg) {
  return sdkMock.get()->opennsl_rx_start(unit, cfg);
}

int __wrap_opennsl_rx_free(int unit, void* pkt_data) {
  return sdkMock.get()->opennsl_rx_free(unit, pkt_data);
}

int __wrap_opennsl_cosq_bst_stat_get(
    int unit,
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosq,
    opennsl_bst_stat_id_t bid,
    uint32 options,
    uint64* value) {
  return sdkMock.get()->opennsl_cosq_bst_stat_get(
      unit, gport, cosq, bid, options, value);
}

int __wrap_opennsl_vlan_default_set(int unit, opennsl_vlan_t vid) {
  return sdkMock.get()->opennsl_vlan_default_set(unit, vid);
}

int __wrap_opennsl_l3_egress_get(
    int unit,
    opennsl_if_t intf,
    opennsl_l3_egress_t* egr) {
  return sdkMock.get()->opennsl_l3_egress_get(unit, intf, egr);
}

int __wrap_opennsl_cosq_bst_stat_clear(
    int unit,
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosq,
    opennsl_bst_stat_id_t bid) {
  return sdkMock.get()->opennsl_cosq_bst_stat_clear(unit, gport, cosq, bid);
}

int __wrap_opennsl_cosq_bst_profile_get(
    int unit,
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosq,
    opennsl_bst_stat_id_t bid,
    opennsl_cosq_bst_profile_t* profile) {
  return sdkMock.get()->opennsl_cosq_bst_profile_get(
      unit, gport, cosq, bid, profile);
}

int __wrap_opennsl_l3_route_max_ecmp_set(int unit, int max) {
  return sdkMock.get()->opennsl_l3_route_max_ecmp_set(unit, max);
}

int __wrap_opennsl_vlan_destroy_all(int unit) {
  return sdkMock.get()->opennsl_vlan_destroy_all(unit);
}

int __wrap_opennsl_detach(int unit) {
  return sdkMock.get()->opennsl_detach(unit);
}

int __wrap_opennsl_switch_control_get(
    int unit,
    opennsl_switch_control_t type,
    int* arg) {
  return sdkMock.get()->opennsl_switch_control_get(unit, type, arg);
}

int __wrap_opennsl_l3_intf_delete(int unit, opennsl_l3_intf_t* intf) {
  return sdkMock.get()->opennsl_l3_intf_delete(unit, intf);
}

int __wrap_opennsl_switch_pkt_trace_info_get(
    int unit,
    uint32 options,
    uint8 port,
    int len,
    uint8* data,
    opennsl_switch_pkt_trace_info_t* pkt_trace_info) {
  return sdkMock.get()->opennsl_switch_pkt_trace_info_get(
      unit, options, port, len, data, pkt_trace_info);
}

int __wrap_opennsl_stg_vlan_add(
    int unit,
    opennsl_stg_t stg,
    opennsl_vlan_t vid) {
  return sdkMock.get()->opennsl_stg_vlan_add(unit, stg, vid);
}

int __wrap_opennsl_l3_egress_destroy(int unit, opennsl_if_t intf) {
  return sdkMock.get()->opennsl_l3_egress_destroy(unit, intf);
}

int __wrap_opennsl_linkscan_detach(int unit) {
  return sdkMock.get()->opennsl_linkscan_detach(unit);
}

int __wrap_opennsl_l3_route_multipath_get(
    int unit,
    opennsl_l3_route_t* the_route,
    opennsl_l3_route_t* path_array,
    int max_path,
    int* path_count) {
  return sdkMock.get()->opennsl_l3_route_multipath_get(
      unit, the_route, path_array, max_path, path_count);
}

int __wrap_opennsl_vlan_list(
    int unit,
    opennsl_vlan_data_t** listp,
    int* countp) {
  return sdkMock.get()->opennsl_vlan_list(unit, listp, countp);
}

int __wrap_opennsl_l3_route_delete_by_interface(
    int unit,
    opennsl_l3_route_t* info) {
  return sdkMock.get()->opennsl_l3_route_delete_by_interface(unit, info);
}

int __wrap_opennsl_port_interface_set(
    int unit,
    opennsl_port_t port,
    opennsl_port_if_t intf) {
  return sdkMock.get()->opennsl_port_interface_set(unit, port, intf);
}

int __wrap_opennsl_tx(int unit, opennsl_pkt_t* tx_pkt, void* cookie) {
  return sdkMock.get()->opennsl_tx(unit, tx_pkt, cookie);
}

int __wrap_opennsl_rx_stop(int unit, opennsl_rx_cfg_t* cfg) {
  return sdkMock.get()->opennsl_rx_stop(unit, cfg);
}

char* __wrap_opennsl_port_name(int unit, int port) {
  return sdkMock.get()->opennsl_port_name(unit, port);
}

int __wrap_opennsl_l3_host_add(int unit, opennsl_l3_host_t* info) {
  return sdkMock.get()->opennsl_l3_host_add(unit, info);
}

int __wrap_opennsl_l2_station_delete(int unit, int station_id) {
  return sdkMock.get()->opennsl_l2_station_delete(unit, station_id);
}

int __wrap_opennsl_stg_destroy(int unit, opennsl_stg_t stg) {
  return sdkMock.get()->opennsl_stg_destroy(unit, stg);
}

int __wrap_opennsl_port_phy_modify(
    int unit,
    opennsl_port_t port,
    uint32 flags,
    uint32 phy_reg_addr,
    uint32 phy_data,
    uint32 phy_mask) {
  return sdkMock.get()->opennsl_port_phy_modify(
      unit, port, flags, phy_reg_addr, phy_data, phy_mask);
}

int __wrap_opennsl_port_speed_max(int unit, opennsl_port_t port, int* speed) {
  return sdkMock.get()->opennsl_port_speed_max(unit, port, speed);
}

int __wrap_opennsl_l2_addr_add(int unit, opennsl_l2_addr_t* l2addr) {
  return sdkMock.get()->opennsl_l2_addr_add(unit, l2addr);
}

int __wrap_opennsl_cosq_bst_stat_multi_get(
    int unit,
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosq,
    uint32 options,
    int max_values,
    opennsl_bst_stat_id_t* id_list,
    uint64* values) {
  return sdkMock.get()->opennsl_cosq_bst_stat_multi_get(
      unit, gport, cosq, options, max_values, id_list, values);
}

void __wrap_opennsl_l3_intf_t_init(opennsl_l3_intf_t* intf) {
  sdkMock.get()->opennsl_l3_intf_t_init(intf);
}

int __wrap_opennsl_rx_unregister(
    int unit,
    opennsl_rx_cb_f callback,
    uint8 priority) {
  return sdkMock.get()->opennsl_rx_unregister(unit, callback, priority);
}

int __wrap_opennsl_l3_intf_get(int unit, opennsl_l3_intf_t* intf) {
  return sdkMock.get()->opennsl_l3_intf_get(unit, intf);
}

void __wrap_opennsl_l3_egress_t_init(opennsl_l3_egress_t* egr) {
  sdkMock.get()->opennsl_l3_egress_t_init(egr);
}

int __wrap_opennsl_attach_check(int unit) {
  return sdkMock.get()->opennsl_attach_check(unit);
}

int __wrap_opennsl_l3_egress_ecmp_traverse(
    int unit,
    opennsl_l3_egress_ecmp_traverse_cb trav_fn,
    void* user_data) {
  return sdkMock.get()->opennsl_l3_egress_ecmp_traverse(
      unit, trav_fn, user_data);
}

int __wrap_opennsl_cosq_bst_profile_set(
    int unit,
    opennsl_gport_t gport,
    opennsl_cos_queue_t cosq,
    opennsl_bst_stat_id_t bid,
    opennsl_cosq_bst_profile_t* profile) {
  return sdkMock.get()->opennsl_cosq_bst_profile_set(
      unit, gport, cosq, bid, profile);
}

int __wrap_opennsl_vlan_destroy(int unit, opennsl_vlan_t vid) {
  return sdkMock.get()->opennsl_vlan_destroy(unit, vid);
}

int __wrap_opennsl_port_vlan_member_set(
    int unit,
    opennsl_port_t port,
    uint32 flags) {
  return sdkMock.get()->opennsl_port_vlan_member_set(unit, port, flags);
}

int __wrap_opennsl_linkscan_unregister(int unit, opennsl_linkscan_handler_t f) {
  return sdkMock.get()->opennsl_linkscan_unregister(unit, f);
}

int __wrap_opennsl_knet_filter_traverse(
    int unit,
    opennsl_knet_filter_traverse_cb trav_fn,
    void* user_data) {
  return sdkMock.get()->opennsl_knet_filter_traverse(unit, trav_fn, user_data);
}

int __wrap_opennsl_knet_netif_destroy(int unit, int netif_id) {
  return sdkMock.get()->opennsl_knet_netif_destroy(unit, netif_id);
}

int __wrap_opennsl_stg_stp_set(
    int unit,
    opennsl_stg_t stg,
    opennsl_port_t port,
    int stp_state) {
  return sdkMock.get()->opennsl_stg_stp_set(unit, stg, port, stp_state);
}

int __wrap_opennsl_rx_cfg_get(int unit, opennsl_rx_cfg_t* cfg) {
  return sdkMock.get()->opennsl_rx_cfg_get(unit, cfg);
}

int __wrap_opennsl_vlan_port_remove(
    int unit,
    opennsl_vlan_t vid,
    opennsl_pbmp_t pbmp) {
  return sdkMock.get()->opennsl_vlan_port_remove(unit, vid, pbmp);
}

int __wrap_opennsl_l3_intf_find_vlan(int unit, opennsl_l3_intf_t* intf) {
  return sdkMock.get()->opennsl_l3_intf_find_vlan(unit, intf);
}

void __wrap_opennsl_l3_route_t_init(opennsl_l3_route_t* route) {
  sdkMock.get()->opennsl_l3_route_t_init(route);
}

void __wrap_opennsl_l2_addr_t_init(
    opennsl_l2_addr_t* l2addr,
    const opennsl_mac_t mac_addr,
    opennsl_vlan_t vid) {
  sdkMock.get()->opennsl_l2_addr_t_init(l2addr, mac_addr, vid);
}

int __wrap_opennsl_l3_egress_multipath_destroy(int unit, opennsl_if_t mpintf) {
  return sdkMock.get()->opennsl_l3_egress_multipath_destroy(unit, mpintf);
}

int __wrap_opennsl_knet_netif_create(int unit, opennsl_knet_netif_t* netif) {
  return sdkMock.get()->opennsl_knet_netif_create(unit, netif);
}

void __wrap_opennsl_knet_filter_t_init(opennsl_knet_filter_t* filter) {
  sdkMock.get()->opennsl_knet_filter_t_init(filter);
}

int __wrap_opennsl_switch_event_unregister(
    int unit,
    opennsl_switch_event_cb_t cb,
    void* userdata) {
  return sdkMock.get()->opennsl_switch_event_unregister(unit, cb, userdata);
}

int __wrap_opennsl_port_link_status_get(
    int unit,
    opennsl_port_t port,
    int* status) {
  return sdkMock.get()->opennsl_port_link_status_get(unit, port, status);
}

int __wrap_opennsl_port_untagged_vlan_get(
    int unit,
    opennsl_port_t port,
    opennsl_vlan_t* vid_ptr) {
  return sdkMock.get()->opennsl_port_untagged_vlan_get(unit, port, vid_ptr);
}

void __wrap_opennsl_knet_netif_t_init(opennsl_knet_netif_t* netif) {
  sdkMock.get()->opennsl_knet_netif_t_init(netif);
}

int __wrap_opennsl_port_stat_enable_set(
    int unit,
    opennsl_gport_t port,
    int enable) {
  return sdkMock.get()->opennsl_port_stat_enable_set(unit, port, enable);
}

int __wrap_opennsl_stat_multi_get(
    int unit,
    opennsl_port_t port,
    int nstat,
    opennsl_stat_val_t* stat_arr,
    uint64* value_arr) {
  return sdkMock.get()->opennsl_stat_multi_get(
      unit, port, nstat, stat_arr, value_arr);
}

int __wrap_opennsl_port_speed_set(int unit, opennsl_port_t port, int speed) {
  return sdkMock.get()->opennsl_port_speed_set(unit, port, speed);
}

int __wrap_opennsl_stg_default_set(int unit, opennsl_stg_t stg) {
  return sdkMock.get()->opennsl_stg_default_set(unit, stg);
}

int __wrap_opennsl_stg_stp_get(
    int unit,
    opennsl_stg_t stg,
    opennsl_port_t port,
    int* stp_state) {
  return sdkMock.get()->opennsl_stg_stp_get(unit, stg, port, stp_state);
}

int __wrap_opennsl_attach_max(int* max_units) {
  return sdkMock.get()->opennsl_attach_max(max_units);
}

int __wrap_opennsl_port_control_set(
    int unit,
    opennsl_port_t port,
    opennsl_port_control_t type,
    int value) {
  return sdkMock.get()->opennsl_port_control_set(unit, port, type, value);
}

int __wrap_opennsl_l3_host_delete(int unit, opennsl_l3_host_t* ip_addr) {
  return sdkMock.get()->opennsl_l3_host_delete(unit, ip_addr);
}

int __wrap_opennsl_knet_filter_create(int unit, opennsl_knet_filter_t* filter) {
  return sdkMock.get()->opennsl_knet_filter_create(unit, filter);
}

int __wrap_opennsl_stg_create(int unit, opennsl_stg_t* stg_ptr) {
  return sdkMock.get()->opennsl_stg_create(unit, stg_ptr);
}

void __wrap_opennsl_l3_egress_ecmp_t_init(opennsl_l3_egress_ecmp_t* ecmp) {
  sdkMock.get()->opennsl_l3_egress_ecmp_t_init(ecmp);
}

int __wrap_opennsl_linkscan_mode_get(int unit, opennsl_port_t port, int* mode) {
  return sdkMock.get()->opennsl_linkscan_mode_get(unit, port, mode);
}

int __wrap_opennsl_switch_control_port_set(
    int unit,
    opennsl_port_t port,
    opennsl_switch_control_t type,
    int arg) {
  return sdkMock.get()->opennsl_switch_control_port_set(unit, port, type, arg);
}

int __wrap_opennsl_linkscan_mode_set_pbm(
    int unit,
    opennsl_pbmp_t pbm,
    int mode) {
  return sdkMock.get()->opennsl_linkscan_mode_set_pbm(unit, pbm, mode);
}

int __wrap_opennsl_l2_station_get(
    int unit,
    int station_id,
    opennsl_l2_station_t* station) {
  return sdkMock.get()->opennsl_l2_station_get(unit, station_id, station);
}

int __wrap_opennsl_port_enable_get(int unit, opennsl_port_t port, int* enable) {
  return sdkMock.get()->opennsl_port_enable_get(unit, port, enable);
}

int __wrap_opennsl_l3_route_max_ecmp_get(int unit, int* max) {
  return sdkMock.get()->opennsl_l3_route_max_ecmp_get(unit, max);
}

int __wrap_opennsl_l3_egress_multipath_find(
    int unit,
    int intf_count,
    opennsl_if_t* intf_array,
    opennsl_if_t* mpintf) {
  return sdkMock.get()->opennsl_l3_egress_multipath_find(
      unit, intf_count, intf_array, mpintf);
}

int __wrap_opennsl_port_local_get(
    int unit,
    opennsl_gport_t gport,
    opennsl_port_t* local_port) {
  return sdkMock.get()->opennsl_port_local_get(unit, gport, local_port);
}

int __wrap_opennsl_stg_default_get(int unit, opennsl_stg_t* stg_ptr) {
  return sdkMock.get()->opennsl_stg_default_get(unit, stg_ptr);
}

int __wrap_opennsl_l3_egress_traverse(
    int unit,
    opennsl_l3_egress_traverse_cb trav_fn,
    void* user_data) {
  return sdkMock.get()->opennsl_l3_egress_traverse(unit, trav_fn, user_data);
}

int __wrap_opennsl_l2_station_add(
    int unit,
    int* station_id,
    opennsl_l2_station_t* station) {
  return sdkMock.get()->opennsl_l2_station_add(unit, station_id, station);
}

int __wrap_opennsl_vlan_create(int unit, opennsl_vlan_t vid) {
  return sdkMock.get()->opennsl_vlan_create(unit, vid);
}

int __wrap_opennsl_cosq_mapping_set(
    int unit,
    opennsl_cos_t priority,
    opennsl_cos_queue_t cosq) {
  return sdkMock.get()->opennsl_cosq_mapping_set(unit, priority, cosq);
}

void __wrap_opennsl_l2_station_t_init(opennsl_l2_station_t* addr) {
  sdkMock.get()->opennsl_l2_station_t_init(addr);
}

int __wrap_opennsl_l3_host_delete_all(int unit, opennsl_l3_host_t* info) {
  return sdkMock.get()->opennsl_l3_host_delete_all(unit, info);
}

int __wrap_opennsl_l3_egress_ecmp_create(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp,
    int intf_count,
    opennsl_if_t* intf_array) {
  return sdkMock.get()->opennsl_l3_egress_ecmp_create(
      unit, ecmp, intf_count, intf_array);
}

int __wrap_opennsl_attach(int unit, char* type, char* subtype, int remunit) {
  return sdkMock.get()->opennsl_attach(unit, type, subtype, remunit);
}

void __wrap_opennsl_l3_host_t_init(opennsl_l3_host_t* ip) {
  sdkMock.get()->opennsl_l3_host_t_init(ip);
}

int __wrap_opennsl_port_vlan_member_get(
    int unit,
    opennsl_port_t port,
    uint32* flags) {
  return sdkMock.get()->opennsl_port_vlan_member_get(unit, port, flags);
}

int __wrap_opennsl_l3_egress_multipath_get(
    int unit,
    opennsl_if_t mpintf,
    int intf_size,
    opennsl_if_t* intf_array,
    int* intf_count) {
  return sdkMock.get()->opennsl_l3_egress_multipath_get(
      unit, mpintf, intf_size, intf_array, intf_count);
}

int __wrap_opennsl_ip6_mask_create(opennsl_ip6_t ip6, int len) {
  return sdkMock.get()->opennsl_ip6_mask_create(ip6, len);
}

int __wrap_opennsl_port_learn_set(int unit, opennsl_port_t port, uint32 flags) {
  return sdkMock.get()->opennsl_port_learn_set(unit, port, flags);
}

int __wrap_opennsl_vlan_gport_delete_all(int unit, opennsl_vlan_t vlan) {
  return sdkMock.get()->opennsl_vlan_gport_delete_all(unit, vlan);
}

int __wrap_opennsl_l3_intf_find(int unit, opennsl_l3_intf_t* intf) {
  return sdkMock.get()->opennsl_l3_intf_find(unit, intf);
}

int __wrap_opennsl_l3_route_delete(int unit, opennsl_l3_route_t* info) {
  return sdkMock.get()->opennsl_l3_route_delete(unit, info);
}

int __wrap_opennsl_linkscan_enable_get(int unit, int* us) {
  return sdkMock.get()->opennsl_linkscan_enable_get(unit, us);
}

int __wrap_opennsl_port_dtag_mode_set(int unit, opennsl_port_t port, int mode) {
  return sdkMock.get()->opennsl_port_dtag_mode_set(unit, port, mode);
}

int __wrap_opennsl_knet_filter_destroy(int unit, int filter_id) {
  return sdkMock.get()->opennsl_knet_filter_destroy(unit, filter_id);
}

int __wrap_opennsl_l3_host_traverse(
    int unit,
    uint32 flags,
    uint32 start,
    uint32 end,
    opennsl_l3_host_traverse_cb cb,
    void* user_data) {
  return sdkMock.get()->opennsl_l3_host_traverse(
      unit, flags, start, end, cb, user_data);
}

int __wrap_opennsl_pkt_alloc(
    int unit,
    int size,
    uint32 flags,
    opennsl_pkt_t** pkt_buf) {
  return sdkMock.get()->opennsl_pkt_alloc(unit, size, flags, pkt_buf);
}

int __wrap_opennsl_linkscan_enable_set(int unit, int us) {
  return sdkMock.get()->opennsl_linkscan_enable_set(unit, us);
}

int __wrap_opennsl_l3_egress_multipath_add(
    int unit,
    opennsl_if_t mpintf,
    opennsl_if_t intf) {
  return sdkMock.get()->opennsl_l3_egress_multipath_add(unit, mpintf, intf);
}

int __wrap_opennsl_l3_ecmp_get(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp_info,
    int ecmp_member_size,
    opennsl_l3_ecmp_member_t* ecmp_member_array,
    int* ecmp_member_count) {
  return sdkMock.get()->opennsl_l3_ecmp_get(
      unit, ecmp_info, ecmp_member_size, ecmp_member_array, ecmp_member_count);
}

int __wrap_opennsl_l3_egress_ecmp_find(
    int unit,
    int intf_count,
    opennsl_if_t* intf_array,
    opennsl_l3_egress_ecmp_t* ecmp) {
  return sdkMock.get()->opennsl_l3_egress_ecmp_find(
      unit, intf_count, intf_array, ecmp);
}

void __wrap_opennsl_port_info_t_init(opennsl_port_info_t* info) {
  sdkMock.get()->opennsl_port_info_t_init(info);
}

int __wrap_opennsl_vlan_port_add(
    int unit,
    opennsl_vlan_t vid,
    opennsl_pbmp_t pbmp,
    opennsl_pbmp_t ubmp) {
  return sdkMock.get()->opennsl_vlan_port_add(unit, vid, pbmp, ubmp);
}

int __wrap_opennsl_port_ability_local_get(
    int unit,
    opennsl_port_t port,
    opennsl_port_ability_t* local_ability_mask) {
  return sdkMock.get()->opennsl_port_ability_local_get(
      unit, port, local_ability_mask);
}

int __wrap_opennsl_l3_egress_create(
    int unit,
    uint32 flags,
    opennsl_l3_egress_t* egr,
    opennsl_if_t* if_id) {
  return sdkMock.get()->opennsl_l3_egress_create(unit, flags, egr, if_id);
}

int __wrap_opennsl_l3_host_delete_by_interface(
    int unit,
    opennsl_l3_host_t* info) {
  return sdkMock.get()->opennsl_l3_host_delete_by_interface(unit, info);
}

int __wrap_opennsl_port_untagged_vlan_set(
    int unit,
    opennsl_port_t port,
    opennsl_vlan_t vid) {
  return sdkMock.get()->opennsl_port_untagged_vlan_set(unit, port, vid);
}

void __wrap_opennsl_port_ability_t_init(opennsl_port_ability_t* ability) {
  sdkMock.get()->opennsl_port_ability_t_init(ability);
}

int __wrap_opennsl_stat_clear(int unit, opennsl_port_t port) {
  return sdkMock.get()->opennsl_stat_clear(unit, port);
}

int __wrap_opennsl_l2_age_timer_set(int unit, int age_seconds) {
  return sdkMock.get()->opennsl_l2_age_timer_set(unit, age_seconds);
}

int __wrap_opennsl_l2_traverse(
    int unit,
    opennsl_l2_traverse_cb trav_fn,
    void* user_data) {
  return sdkMock.get()->opennsl_l2_traverse(unit, trav_fn, user_data);
}

int __wrap_opennsl_knet_netif_traverse(
    int unit,
    opennsl_knet_netif_traverse_cb trav_fn,
    void* user_data) {
  return sdkMock.get()->opennsl_knet_netif_traverse(unit, trav_fn, user_data);
}

int __wrap_opennsl_port_enable_set(int unit, opennsl_port_t port, int enable) {
  return sdkMock.get()->opennsl_port_enable_set(unit, port, enable);
}

int __wrap_opennsl_l3_egress_ecmp_destroy(
    int unit,
    opennsl_l3_egress_ecmp_t* ecmp) {
  return sdkMock.get()->opennsl_l3_egress_ecmp_destroy(unit, ecmp);
}

int __wrap_opennsl_port_interface_get(
    int unit,
    opennsl_port_t port,
    opennsl_port_if_t* intf) {
  return sdkMock.get()->opennsl_port_interface_get(unit, port, intf);
}

int __wrap_opennsl_knet_init(int unit) {
  return sdkMock.get()->opennsl_knet_init(unit);
}

int __wrap_opennsl_l3_egress_multipath_delete(
    int unit,
    opennsl_if_t mpintf,
    opennsl_if_t intf) {
  return sdkMock.get()->opennsl_l3_egress_multipath_delete(unit, mpintf, intf);
}

int __wrap_opennsl_l3_route_add(int unit, opennsl_l3_route_t* info) {
  return sdkMock.get()->opennsl_l3_route_add(unit, info);
}

int __wrap_opennsl_l3_route_traverse(
    int unit,
    uint32 flags,
    uint32 start,
    uint32 end,
    opennsl_l3_route_traverse_cb trav_fn,
    void* user_data) {
  return sdkMock.get()->opennsl_l3_route_traverse(
      unit, flags, start, end, trav_fn, user_data);
}

int __wrap_opennsl_vlan_list_destroy(
    int unit,
    opennsl_vlan_data_t* list,
    int count) {
  return sdkMock.get()->opennsl_vlan_list_destroy(unit, list, count);
}

int __wrap_opennsl_l2_addr_get(
    int unit,
    opennsl_mac_t mac_addr,
    opennsl_vlan_t vid,
    opennsl_l2_addr_t* l2addr) {
  return sdkMock.get()->opennsl_l2_addr_get(unit, mac_addr, vid, l2addr);
}

int __wrap_opennsl_l3_egress_multipath_traverse(
    int unit,
    opennsl_l3_egress_multipath_traverse_cb trav_fn,
    void* user_data) {
  return sdkMock.get()->opennsl_l3_egress_multipath_traverse(
      unit, trav_fn, user_data);
}

int __wrap_opennsl_l2_addr_delete(
    int unit,
    opennsl_mac_t mac,
    opennsl_vlan_t vid) {
  return sdkMock.get()->opennsl_l2_addr_delete(unit, mac, vid);
}

int __wrap_opennsl_pkt_flags_init(
    int unit,
    opennsl_pkt_t* pkt,
    uint32 init_flags) {
  return sdkMock.get()->opennsl_pkt_flags_init(unit, pkt, init_flags);
}

int __wrap_opennsl_pkt_free(int unit, opennsl_pkt_t* pkt) {
  return sdkMock.get()->opennsl_pkt_free(unit, pkt);
}

int __wrap_opennsl_l3_info(int unit, opennsl_l3_info_t* l3info) {
  return sdkMock.get()->opennsl_l3_info(unit, l3info);
}

int __wrap_opennsl_l3_route_get(int unit, opennsl_l3_route_t* info) {
  return sdkMock.get()->opennsl_l3_route_get(unit, info);
}

int __wrap_opennsl_l3_egress_multipath_create(
    int unit,
    uint32 flags,
    int intf_count,
    opennsl_if_t* intf_array,
    opennsl_if_t* mpintf) {
  return sdkMock.get()->opennsl_l3_egress_multipath_create(
      unit, flags, intf_count, intf_array, mpintf);
}

int __wrap_opennsl_cosq_mapping_get(
    int unit,
    opennsl_cos_t priority,
    opennsl_cos_queue_t* cosq) {
  return sdkMock.get()->opennsl_cosq_mapping_get(unit, priority, cosq);
}

int __wrap_opennsl_port_ability_advert_set(
    int unit,
    opennsl_port_t port,
    opennsl_port_ability_t* ability_mask) {
  return sdkMock.get()->opennsl_port_ability_advert_set(
      unit, port, ability_mask);
}

int __wrap_opennsl_switch_control_set(
    int unit,
    opennsl_switch_control_t type,
    int arg) {
  return sdkMock.get()->opennsl_switch_control_set(unit, type, arg);
}

int __wrap_opennsl_switch_event_register(
    int unit,
    opennsl_switch_event_cb_t cb,
    void* userdata) {
  return sdkMock.get()->opennsl_switch_event_register(unit, cb, userdata);
}

int __wrap_opennsl_port_frame_max_get(
    int unit,
    opennsl_port_t port,
    int* size) {
  return sdkMock.get()->opennsl_port_frame_max_get(unit, port, size);
}

int __wrap_opennsl_linkscan_mode_set(int unit, opennsl_port_t port, int mode) {
  return sdkMock.get()->opennsl_linkscan_mode_set(unit, port, mode);
}

int __wrap_opennsl_l3_intf_create(int unit, opennsl_l3_intf_t* intf) {
  return sdkMock.get()->opennsl_l3_intf_create(unit, intf);
}

int __wrap_opennsl_rx_control_get(
    int unit,
    opennsl_rx_control_t type,
    int* arg) {
  return sdkMock.get()->opennsl_rx_control_get(unit, type, arg);
}

void __wrap_opennsl_l3_info_t_init(opennsl_l3_info_t* info) {
  sdkMock.get()->opennsl_l3_info_t_init(info);
}

int __wrap_opennsl_l2_age_timer_get(int unit, int* age_seconds) {
  return sdkMock.get()->opennsl_l2_age_timer_get(unit, age_seconds);
}

int __wrap_opennsl_l3_host_find(int unit, opennsl_l3_host_t* info) {
  return sdkMock.get()->opennsl_l3_host_find(unit, info);
}

int __wrap_opennsl_switch_control_port_get(
    int unit,
    opennsl_port_t port,
    opennsl_switch_control_t type,
    int* arg) {
  return sdkMock.get()->opennsl_switch_control_port_get(unit, port, type, arg);
}

int __wrap_opennsl_port_control_get(
    int unit,
    opennsl_port_t port,
    opennsl_port_control_t type,
    int* value) {
  return sdkMock.get()->opennsl_port_control_get(unit, port, type, value);
}

int __wrap_opennsl_port_selective_get(
    int unit,
    opennsl_port_t port,
    opennsl_port_info_t* info) {
  return sdkMock.get()->opennsl_port_selective_get(unit, port, info);
}

int __wrap_opennsl_port_frame_max_set(int unit, opennsl_port_t port, int size) {
  return sdkMock.get()->opennsl_port_frame_max_set(unit, port, size);
}

int __wrap_opennsl_vlan_default_get(int unit, opennsl_vlan_t* vid_ptr) {
  return sdkMock.get()->opennsl_vlan_default_get(unit, vid_ptr);
}
}
