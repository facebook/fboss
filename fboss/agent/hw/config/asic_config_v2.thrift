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
  2: optional map<i64, AsicConfigEntry> npuEntries;
}

/*
 * AsicConfigGenType determines the vendor configs to be
 * generated based on the gen type mode.
 * - DEFAULT - prod asic config
 * - HW_TEST - SAI Hw test asic config
 * - BENCHMARK - Benchmark asic config
 * - LINK_TEST - Link test asic config
 */
enum AsicConfigGenType {
  DEFAULT = 0,
  HW_TEST = 1,
  BENCHMARK = 2,
  LINK_TEST = 3,
}

// asic config depending on roles in multistage DNX network
enum MultistageRole {
  NONE = 0,
  FAP = 1, // for Jericho3
  FE13 = 2, // for 1st stage Ramon3
  FE2 = 3, // for 2nd stage Ramon3
  FE13_BEEP = 4, // for beep layer 1st stage Ramon3
}

struct AsicConfigParameters {
  1: AsicConfigType configType;
  2: optional bool exactMatch;
  3: optional bool mmuLossless;
  4: optional AsicConfigGenType configGenType;
  5: optional string portConfig;
  6: optional MultistageRole multistageRole;
  7: optional bool hyperPort;
}

struct AsicVendorConfigParams {
  1: map<string, string> commonConfig;
  2: map<string, string> prodConfig;
  3: map<string, string> hwTestConfig;
  4: map<string, string> linkTestConfig;
  5: map<string, string> benchmarkConfig;
  6: map<string, string> portMapConfig;
  7: map<string, string> multistageConfig;
}
