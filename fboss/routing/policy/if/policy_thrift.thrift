// Routing Policy Types - Minimal stub for FBOSS OSS
// Contains types used by bgp_thrift.thrift

namespace cpp2 facebook.neteng.routing.policy.thrift
namespace py neteng.routing.policy_thrift
namespace py3 neteng.routing.policy_thrift

// Policy Statistics
// Used by: getPolicyStats() in TBgpService
struct TPolicyStats {
  1: i64 routes_matched;
  2: i64 routes_rejected;
  3: optional map<string, i64> policy_counters;
}
