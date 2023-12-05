namespace cpp2 facebook.fboss.platform.data_corral_service
namespace py3 neteng.fboss.platform.data_corral_service

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

service DataCorralServiceThrift {
  // if parameter "uncached" is true, then fruid should read from
  // flash/eeprom instead of cached value
  DataCorralFruidReadResponse getFruid(1: bool uncached) throws (
    1: fboss.FbossBaseError error,
  );
}
