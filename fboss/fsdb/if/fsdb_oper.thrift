namespace py3 neteng.fboss
namespace py neteng.fboss.fsdb_oper
namespace py.asyncio neteng.fboss.asyncio.fsdb_oper
namespace cpp2 facebook.fboss.fsdb
namespace go facebook.fboss.fsdb_oper

include "fboss/fsdb/if/fsdb_common.thrift"

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
  3: optional OperMetadata metadata;
}

struct OperPubRequest {
  1: fsdb_oper.OperPath path;
  2: fsdb_common.PublisherId publisherId;
}

struct OperPubInitResponse {}

struct OperPubFinalResponse {}

struct OperSubRequest {
  1: fsdb_oper.OperPath path;
  2: fsdb_oper.OperProtocol protocol = fsdb_oper.OperProtocol.BINARY;
}

struct OperSubInitResponse {}
