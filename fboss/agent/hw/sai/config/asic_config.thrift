namespace py neteng.fboss.asic_config
namespace py3 neteng.fboss.asic_config
namespace py.asyncio neteng.fboss.asyncio.asic_config
namespace cpp2 facebook.fboss.asic
namespace go neteng.fboss.asic_config
namespace php fboss_asic_config

struct SerdesParam {
  1: byte slice_id
  2: byte ifg_id
  3: i16 speed
  4: string media_type
  5: i16 TX_PRE1
  6: i16 TX_MAIN
  7: i16 TX_POST
  8: i16 RX_CTLE_CODE
  9: byte RX_DSP_MODE
  10: byte RX_AFE_TRIM
  11: byte RX_AC_COUPLING_BYPASS
}

struct SerdesParams {
  1: string VERSION
  2: map<string, SerdesParam> params
}

struct IFGSwap {
  1: byte slice
  2: byte ifg
  3: list<byte> swap
  4: list<byte> serdes_polarity_inverse_rx
  5: list<byte> serdes_polarity_inverse_tx
}

struct Device {
  1: i16 id
  2: string type
  3: i16 rev
  4: list<IFGSwap> ifg_swap_lists
  5: SerdesParams serdes_params
}


struct DeviceConfig {
  1: string board_type
  2: string description
  3: string board_rev
  4: list<Device> devices
}

struct AsicConfig {
  1: string config
}
