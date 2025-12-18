include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

namespace py neteng.fboss.asic_config
namespace py3 neteng.fboss.asic_config
namespace py.asyncio neteng.fboss.asyncio.asic_config
namespace cpp2 facebook.fboss.asic
namespace go neteng.fboss.asic_config
namespace php fboss_asic_config

struct AsicRxSettings {
  1: string stage;
  2: string mode;
}

struct RxSettingsParams {
  1: list<AsicRxSettings> RX_AFE_TRIM;
}

struct DefaultParams {
  1: i32 RX_AFE_TRIM;
}

struct CliServer {
  1: string address;
  2: bool enable;
}

struct SerdesParams {
  1: list<AsicRxSettings> param_stages_10G_DEPRECATED;
  2: list<AsicRxSettings> param_stages_25G_DEPRECATED;
  3: list<AsicRxSettings> param_stages_50G_DEPRECATED;
  4: RxSettingsParams param_stages_default;
  5: DefaultParams default_params;
}

struct IFGSwap {
  1: byte slice;
  2: byte ifg;
  3: list<byte> swap;
  4: list<byte> serdes_polarity_inverse_rx;
  5: list<byte> serdes_polarity_inverse_tx;
}

struct DeviceProperty {
  1: bool poll_msi;
  2: i32 device_frequency;
  3: bool flow_cache_enable;
  4: bool allow_smac_equals_dmac;
  5: optional i32 ref_clk_pll_serdes;
  /*
   * Enable ipv6 sip/dip compression for ACL
   */
  6: optional bool enable_acl_sip_dip_compression;
  /*
   * SDK reserves certain number of policers for LPTS logic.
   * Suggest from the vendor is to set this global device property
   * LPTS_MAX_ENTRY_COUNTERS=16
   */
  7: optional i32 lpts_max_entry_counters;
  /*
   *  When enabled, the ecmp combine with member MPLS/non MPLS
   * joining in any sequence always use L1 ecmp
   */
  8: optional bool force_l1_ecmp;
  /*
   * When enabled, the route entry result will contain
   * counter,but we need to set the cem_num_banks to
   * smaller number number 12 to accommodate it.
   */
  9: optional bool enable_forwarding_route_counter;
  10: optional i32 cem_num_banks;
  /*
   * When enabled,  we will wait until ACL result for lpts hit packet.
   * If ACL result is matched, then use ACL result else use lpts result
   */
  11: optional bool trap_lpts_after_post_fwd_acl;
  /*
   * Enable 2 counters for ACL
   */
  12: optional bool enable_ingress_l2_port_counters;
  13: optional bool enable_ingress_tunnel_counters;
  14: optional bool enable_ingress_2_acl_counters;
  /*
   * When enabled, queue ID will be set in the RX packet
   */
  15: optional bool use_original_tc_for_punt;
  /*
   * Must be used only in tests to flush the programming
   * state before sending packets from CPU
   */
  16: optional bool flush_before_inject;

  /*
   * Certain asics have HBM (High Bandwith Memory),
   * eg: cloudripper. Forcefully disable HBM
   * usage on these devices. MUST be used for
   * tests only
   */
  17: optional bool force_disable_hbm;

  /*
   * When enabled, checking FEC histogram to detect bad link.
   * If a faulty link is detected, the port's Link state
   * will set to "down".
   */
  18: optional bool enable_mac_port_bad_link_check;
}

struct Device {
  1: i16 id;
  2: string type;
  3: i16 rev;
  4: list<IFGSwap> ifg_swap_lists;
  5: optional SerdesParams serdes_params;
  6: DeviceProperty device_property;
  7: optional i32 slice_mode_bitmap;
  8: optional string fabric_mode;
}

struct DeviceConfig {
  1: string board_type;
  2: string description;
  3: string board_rev;
  4: list<Device> devices;
  5: optional CliServer cli_server;
}

struct AsicConfig {
  1: string config;
}
