namespace cpp2 facebook.fboss.fsdb
namespace py neteng.fboss.fsdb.fsdb_config
namespace py3 neteng.fboss.fsdb
namespace py.asyncio neteng.fboss.asyncio.fsdb_config

include "fboss/fsdb/if/fsdb_common.thrift"
include "fboss/fsdb/if/fsdb_oper.thrift"

struct Config {
  1: map<string, string> (
    cpp.template = 'std::unordered_map',
  ) defaultCommandLineArgs;
  2: map<fsdb_common.PublisherId, list<fsdb_oper.OperPath>> (
    cpp.template = 'std::unordered_map',
  ) operPublishers = {};
}
