namespace cpp2 facebook.fboss.platform.data_corral_service
namespace go neteng.fboss.platform.data_corral_service
namespace py neteng.fboss.platform.data_corral_service
namespace py3 neteng.fboss.platform.data_corral_service
namespace py.asyncio neteng.fboss.platform.asyncio.data_corral_service

include "fboss/agent/if/fboss.thrift"

// All timestamps are Epoch time in second
struct FruIdData {
  1: string name;
  2: string value;
}

// Send back fruid (same as output from weutil) data based on request
struct DataCorralFruidReadResponse {
  1: list<FruIdData> fruidData;
}

// module attribute and the corresponding sysfs path
struct AttributeConfig {
  1: string name;
  2: string path;
}

// Platform config info of a fru module
struct DataCorralFruConfig {
  1: string name;
  2: list<AttributeConfig> attributes;
}

// Platform config info of the whole chassis
struct DataCorralPlatformConfig {
  1: list<DataCorralFruConfig> fruModules;
  2: list<AttributeConfig> chassisAttributes;
}

service DataCorralServiceThrift {
  // if parameter "uncached" is true, then fruid should read from
  // flash/eeprom instead of cached value
  DataCorralFruidReadResponse getFruid(1: bool uncached) throws (
    1: fboss.FbossBaseError error,
  ) (cpp.coroutine);
}
