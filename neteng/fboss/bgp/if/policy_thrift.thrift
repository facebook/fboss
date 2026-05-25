namespace py neteng.routing.policy_thrift
namespace py.asyncio neteng.routing.asyncio.policy_thrift
namespace py3 neteng.routing
namespace cpp2 facebook.neteng.routing.policy.thrift
namespace go neteng.routing.policy_thrift

include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

/**
* Per term stats
*/
struct TPolicyTermStats {
  1: string name = "";
  2: string description = "";
  3: i64 prefix_hit_count; // Number of prefix hits for this term
}

/**
* Per policy statement stats
*/
struct TPolicyStatementStats {
  1: string name = "";
  @thrift.AllowUnsafeOptionalCustomDefaultValue
  2: optional string description = "";
  3: i64 prefix_hit_count; // Total number of prefixes executed for this policy
  4: i64 avg_time; // Avg time (us) spent on policy
  5: i64 max_time; // Max time (us) spent on policy
  6: i64 num_of_runs; // Number of runs of this policy
  7: list<TPolicyTermStats> term_stats;
}

/**
* Get policy stats
*/
struct TPolicyStats {
  1: list<TPolicyStatementStats> policy_statement_stats;
}
