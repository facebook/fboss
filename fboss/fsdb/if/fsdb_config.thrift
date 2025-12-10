package "facebook.com/fboss/fsdb"

namespace cpp2 facebook.fboss.fsdb
namespace py neteng.fboss.fsdb.fsdb_config
namespace py3 neteng.fboss.fsdb
namespace py.asyncio neteng.fboss.asyncio.fsdb_config

include "fboss/fsdb/if/fsdb_common.thrift"
include "fboss/fsdb/if/fsdb_oper.thrift"
include "thrift/annotation/cpp.thrift"

struct PathConfig {
  1: fsdb_oper.OperPath path;
  // isExpected: for FSDB stats, whether stream is expected to be connected on this path
  2: bool isExpected = false;
  3: bool isStats = false;
}

struct PublisherConfig {
  1: list<PathConfig> paths = [];
  // skipThriftStreamLivenessCheck for Publisher stream while serving subscriptions
  2: bool skipThriftStreamLivenessCheck = false;
}

struct SubscriberConfig {
  1: bool trackReconnect = true;
  2: bool allowExtendedSubscriptions = false;
  3: i16 numExpectedSubscriptions = 1;
}

struct Config {
  @cpp.Type{template = "std::unordered_map"}
  1: map<string, string> defaultCommandLineArgs;
  @cpp.Type{template = "std::unordered_map"}
  // operPublishers is replaced by publishers map.
  2: map<
    fsdb_common.PublisherId,
    list<fsdb_oper.OperPath>
  > operPublishers_DEPRECATED = {};
  // trustedSubnets in CIDRNetwork format. Used for server side enforcement of transport privileges.
  // only clients from trustedSubnets are allowed to connect with TC marking enabled
  3: list<string> trustedSubnets = [];
  @cpp.Type{template = "std::unordered_map"}
  4: map<fsdb_common.PublisherId, PublisherConfig> publishers = {};
  // subscribers{} allows SubscriberConfig to be specified for dynamic
  // SubscriberId strings with a common suffix starting with ":", e.g.
  // ":agent" will match all SubscriberId strings that end with that suffix.
  @cpp.Type{template = "std::unordered_map"}
  5: map<fsdb_common.SubscriberId, SubscriberConfig> subscribers = {};
}
