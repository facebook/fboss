include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

namespace cpp2 facebook.fboss.portstate
namespace go neteng.fboss.port_state
namespace php fboss_common
namespace py neteng.fboss.port_state
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.port_state

struct PortState {
  1: i64 tcvrProgrammingStartTs = 0;
  2: i64 tcvrProgrammingCompleteTs = 0;
}
