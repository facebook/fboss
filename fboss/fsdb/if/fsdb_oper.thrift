namespace py3 neteng.fboss
namespace py neteng.fboss.fsdb_oper
namespace py.asyncio neteng.fboss.asyncio.fsdb_oper
namespace cpp2 facebook.fboss.fsdb
namespace go facebook.fboss.fsdb_oper

include "fboss/agent/agent_config.thrift"

typedef binary (cpp2.type = "::folly::fbstring") fbbinary

/*
 * Generic types for interacting w/ OperState
 */

struct OperPath {
  1: list<string> raw;
// TODO: path extensions here like regex support
}

enum OperProtocol {
  BINARY = 1,
  SIMPLE_JSON = 2,
  COMPACT = 3,
}

struct OperMetadata {
  1: i64 generation;
}

struct OperState {
  1: fbbinary contents;
  2: OperProtocol protocol;
  3: optional OperMetadata metadata;
}

struct TaggedOperState {
  1: OperPath path;
  2: OperState state;
}

struct OperDeltaUnit {
  1: OperPath path;
  2: optional fbbinary oldState;
  3: optional fbbinary newState;
}

struct OperDelta {
  1: list<OperDeltaUnit> changes;
  2: OperProtocol protocol;
}

/*
 * The MODEL for the operational state should be defined below this line.
 */

struct AgentData {
  1: agent_config.AgentConfig config;
}

struct BgpData {}

struct QsfpServiceData {}

struct OpenrData {}

struct FsdbOperRoot {
  1: AgentData agent;
  2: BgpData bgp;
  3: OpenrData openr;
  4: QsfpServiceData qsfp_service;
} (thriftpath.root)
