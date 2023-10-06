namespace py3 neteng.fboss
namespace py neteng.fboss.fsdb_model
namespace cpp2 facebook.fboss.fsdb
namespace go facebook.fboss.fsdb

include "fboss/agent/agent_config.thrift"
include "fboss/agent/agent_stats.thrift"
include "fboss/agent/switch_state.thrift"
include "fboss/qsfp_service/if/qsfp_state.thrift"
include "fboss/qsfp_service/if/qsfp_stats.thrift"
include "fboss/fsdb/if/fsdb_common.thrift"
include "fboss/platform/sensor_service/sensor_service_stats.thrift"

// mirrors of structs fboss/fsdb/if/facebook/fsdb_model.thrift with oss only portions
struct AgentData {
  1: agent_config.AgentConfig config;
  2: switch_state.SwitchState switchState;
  4: map<string, fsdb_common.FsdbSubscriptionState> fsdbSubscriptions = {};
}

struct FsdbOperStateRoot {
  1: AgentData agent;
  4: qsfp_state.QsfpServiceData qsfp_service;
} (thriftpath.root)

struct FsdbOperStatsRoot {
  1: agent_stats.AgentStats agent;
  3: qsfp_stats.QsfpStats qsfp_service;
  4: sensor_service_stats.SensorServiceStats sensor_service;
} (thriftpath.root)
