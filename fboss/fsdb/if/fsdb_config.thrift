namespace cpp2 facebook.fboss.fsdb
namespace py neteng.fboss.fsdb.fsdb_config
namespace py3 neteng.fboss.fsdb
namespace py.asyncio neteng.fboss.asyncio.fsdb_config

include "fboss/fsdb/if/fsdb_common.thrift"
include "fboss/fsdb/if/fsdb_oper.thrift"
include "thrift/annotation/cpp.thrift"

struct PathConfig {
  1: fsdb_oper.OperPath path;
  2: bool isExpected; // whether stream is expected to be connected on this path in healthy state
}

struct PublisherConfig {
  1: list<PathConfig> paths = [];
}

struct Config {
  @cpp.Type{template = "std::unordered_map"}
  1: map<string, string> defaultCommandLineArgs;
  // TODO: deprecate operPublishers config in lieu of publishers{} after 1 push.
  @cpp.Type{template = "std::unordered_map"}
  2: map<fsdb_common.PublisherId, list<fsdb_oper.OperPath>> operPublishers = {};
  // trustedSubnets in CIDRNetwork format. Used for server side enforcement of transport privileges.
  // only clients from trustedSubnets are allowed to connect with TC marking enabled
  3: list<string> trustedSubnets = [];
  @cpp.Type{template = "std::unordered_map"}
  4: map<fsdb_common.PublisherId, PublisherConfig> publishers = {};
}
