namespace py neteng.fboss.asic_config_v2
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.asic_config_v2
namespace cpp2 facebook.fboss.cfg
namespace go neteng.fboss.asic_config_v2
namespace php fboss_asic_config_v2

enum AsicConfigType {
  NONE = 0,
  KEY_VALUE_CONFIG = 1,
  JSON_CONFIG = 2,
  YAML_CONFIG = 3,
}

union AsicConfigEntry {
  1: map<string, string> config;
  2: string jsonConfig;
  3: string yamlConfig;
}

struct AsicConfig {
  1: AsicConfigEntry common;
  2: optional map<i64, AsicConfigEntry> multinpu;
}

struct AsicConfigParameters {
  1: AsicConfigType configType;
  2: optional bool exactMatch;
  3: optional bool mmuLossless;
  4: optional bool testConfig;
}
