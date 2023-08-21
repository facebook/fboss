namespace py3 neteng.fboss
namespace py neteng.fboss.fsdb_model
namespace cpp2 facebook.fboss.fsdb
namespace go facebook.fboss.fsdb

include "fboss/agent/agent_config.thrift"
include "fboss/agent/agent_stats.thrift"
include "fboss/agent/switch_state.thrift"

// mirrors of structs fboss/fsdb/if/facebook/fsdb_model.thrift with oss only portions
struct AgentData {
  1: agent_config.AgentConfig config;
  2: switch_state.SwitchState switchState;
}

struct FsdbOperStateRoot {
  1: AgentData agent;
} (thriftpath.root)

struct FsdbOperStatsRoot {
  1: agent_stats.AgentStats agent;
} (thriftpath.root)
