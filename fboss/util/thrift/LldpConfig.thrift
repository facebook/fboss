namespace cpp facebook.fboss.cfg
namespace cpp2 facebook.fboss.cfg
namespace py neteng.fboss.config

enum PortMode {
  // The numbers used here are the same as in Broadcom code
  DEFAULT = 0,
  XGMII = 6,
  KR4 = 12,
  CR4 = 14,
  SR4 = 28,
}

struct OnePortConfig {
  1: i32 portId,
  2: PortMode portMode,
  3: optional i64 portSpeedMbps,
}

struct LldpConfig {
  1: map<i32, OnePortConfig> portConfigMap,
}
