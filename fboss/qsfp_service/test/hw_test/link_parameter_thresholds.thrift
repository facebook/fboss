namespace cpp2 facebook.fboss
namespace go neteng.fboss.link_parameter_thresholds
namespace py3 neteng.fboss
namespace py neteng.fboss.link_parameter_thresholds
namespace py.asyncio neteng.fboss.asyncio.link_parameter_thresholds

include "fboss/qsfp_service/if/transceiver.thrift"

enum ModuleConnection {
  PLUGGABLE = 1,
  ON_BOARD = 2,
}

struct TransceiverThresholds {
  1: double tx_power_min_dbm;
  2: double tx_power_max_dbm;
  3: double tx_power_fluctuation_dbm;
  4: double tx_bias_min_ma;
  5: double rx_power_min_dbm;
  6: double rx_power_max_dbm;
  7: double rx_power_fluctuation_dbm;
  8: double rx_power_channel_variation_dbm;
  9: double media_snr_min_db;
  10: double insertion_loss_dbm;
  11: double temperature_min_celsius;
  12: double temperature_max_celsius;
  13: double vcc_min_v;
  14: double vcc_max_v;
  15: double insertion_loss_variation_dbm;
}

struct LinkParametersMap {
  1: map<
    transceiver.TransceiverModuleIdentifier,
    ModuleConnection
  > module_connections;
  2: map<
    transceiver.MediaInterfaceCode,
    map<ModuleConnection, TransceiverThresholds>
  > thresholds;
}
