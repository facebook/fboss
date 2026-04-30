// VIP Service Configuration - Minimal stub for FBOSS OSS
// Contains only the types referenced by bgp_config.thrift

namespace cpp2 neteng.config.vip_service_config
namespace py neteng.config.vip_service_config
namespace py3 neteng.config.vip_service_config

// VIP Service Configuration
// Used by: BgpConfig.vip_service_config in bgp_config.thrift
struct VipServiceConfig {
  1: optional string service_name;
  2: optional i32 port;
  3: optional map<string, string> config_params;
}
