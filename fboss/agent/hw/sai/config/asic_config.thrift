namespace py neteng.fboss.asic_config
namespace py3 neteng.fboss.asic_config
namespace py.asyncio neteng.fboss.asyncio.asic_config
namespace cpp2 facebook.fboss.asic
namespace go neteng.fboss.asic_config
namespace php fboss_asic_config

struct SerdesParams {
  1: string VERSION
}

struct IFGSwap {
  1: byte slice
  2: byte ifg
  3: list<byte> swap
  4: list<byte> serdes_polarity_inverse_rx
  5: list<byte> serdes_polarity_inverse_tx
}

struct DeviceProperty {
  1: bool poll_msi
  2: i32 device_frequency
}

struct Device {
  1: i16 id
  2: string type
  3: i16 rev
  4: list<IFGSwap> ifg_swap_lists
  5: optional SerdesParams serdes_params_DEPRECATED
  6: DeviceProperty device_property
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
