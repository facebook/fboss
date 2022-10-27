namespace py neteng.fboss.asic_config
namespace py3 neteng.fboss.asic_config
namespace py.asyncio neteng.fboss.asyncio.asic_config
namespace cpp2 facebook.fboss.asic
namespace go neteng.fboss.asic_config
namespace php fboss_asic_config

struct RxSettings {
  1: string stage;
  2: string mode;
}

struct RxSettingsParams {
  1: list<RxSettings> RX_AFE_TRIM;
}

struct DefaultParams {
  1: i32 RX_AFE_TRIM;
}

struct SerdesParams {
  1: list<RxSettings> param_stages_10G_DEPRECATED;
  2: list<RxSettings> param_stages_25G_DEPRECATED;
  3: list<RxSettings> param_stages_50G_DEPRECATED;
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
  6: optional bool enable_acl_sip_dip_compression;
}

struct Device {
  1: i16 id;
  2: string type;
  3: i16 rev;
  4: list<IFGSwap> ifg_swap_lists;
  5: optional SerdesParams serdes_params;
  6: DeviceProperty device_property;
}

struct DeviceConfig {
  1: string board_type;
  2: string description;
  3: string board_rev;
  4: list<Device> devices;
}

struct AsicConfig {
  1: string config;
}
