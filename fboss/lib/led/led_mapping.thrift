include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

namespace py3 neteng.fboss.lib.led
namespace cpp2 facebook.fboss
namespace py neteng.fboss.lib.led.led_mapping

struct LedMapping {
  1: i32 id;
  2: optional string bluePath;
  3: optional string yellowPath;
  4: i32 transceiverId;
}
