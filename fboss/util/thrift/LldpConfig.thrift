namespace cpp2 facebook.fboss.cfg
namespace py neteng.fboss.config

enum PortMode {
  // The numbers used here are the same as in Broadcom code
  DEFAULT = 0,
  CR4 = 14,
  CAUI = 26,
}

struct OnePortConfig {
  1: i32 portId,
  2: PortMode portMode,
  3: optional i64 portSpeedMbps,
  // FEC needs to be enabled for 100G optic ports
  // disableFEC overrides any speed based decisions.
  4: optional bool disableFEC,
}

struct LldpConfig {
  1: map<i32, OnePortConfig> portConfigMap,
}
