namespace cpp2 facebook.fboss.platform.misc_service
namespace go neteng.fboss.platform.misc_service
namespace py neteng.fboss.platform.misc_service
namespace py3 neteng.fboss.platform.misc_service
namespace py.asyncio neteng.fboss.platform.asyncio.misc_service

include "fboss/agent/if/fboss.thrift"

// All timestamps are Epoch time in second
struct FruIdData {
  1: string name;
  2: string value;
}

// Send back fruid (same as output from weutil) data based on request
struct MiscFruidReadResponse {
  1: list<FruIdData> fruidData;
}

service MiscServiceThrift {
  // if parameter "uncached" is true, then fruid should read from
  // flash/eeprom instead of cached value
  MiscFruidReadResponse getFruid(1: bool uncached) throws (
    1: fboss.FbossBaseError error,
  ) (cpp.coroutine);
}
