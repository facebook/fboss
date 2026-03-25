namespace cpp2 facebook.fboss.asic_temp
namespace go neteng.fboss.asic_temp
namespace php asic_temp
namespace py neteng.fboss.asic_temp
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.asic_temp

include "fboss/agent/if/fboss.thrift"
include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

struct AsicTempData {
  1: string name;
  2: optional float value;
  3: optional i64 timeStamp;
}

struct AsicTempResponse {
  1: i64 timeStamp;
  2: list<AsicTempData> asicTempData;
}

service AsicTempThrift {
  AsicTempResponse getAsicTemp(1: list<string> sensorNames) throws (
    1: fboss.FbossBaseError error,
  );
}
