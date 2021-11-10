namespace cpp2 facebook.fboss.platform.rackmon
namespace go neteng.fboss.platform.rackmon
namespace py neteng.fboss.platform.rackmon
namespace py3 neteng.fboss.platform.rackmon
namespace py.asyncio neteng.fboss.platform.asyncio.rackmon
namespace rust facebook.fboss.platform.rackmon

include "common/fb303/if/fb303.thrift"
include "fboss/agent/if/fboss.thrift"

struct PsuDevice {
  1: i32 address;
  2: i32 crc_errors;
  3: i32 timeouts;
  4: i32 baudrate;
  5: optional string name;
}

struct ModbusRequest {
  1: i32 reqLength;
  2: i32 respLength;
  3: optional i32 timeout;
  4: list<byte> reqCmd;
}

struct ModbusResponse {
  1: i32 status;
  2: i32 respLength;
  3: optional list<byte> response;
}

enum RackmonRequestType {
  /* Trigger rackmond PSU rescan as soon as possible. */
  FORCE_PSU_RESCAN = 0,

  /* Pause rackmond core loop. */
  PAUSE_RACKMOND = 1,

  /* Resume rackmond core loop. */
  RESUME_RACKMOND = 2,
}

struct RackmonResponse {
  1: RackmonRequestType type;
  2: i32 status;
}

service RackmonCtrl extends fb303.FacebookService {
  list<PsuDevice> getPsuDevices() throws (1: fboss.FbossBaseError error);

  ModbusResponse rawModbusCmd(1: ModbusRequest req) throws (
    1: fboss.FbossBaseError error,
  );

  RackmonResponse configRackmond(1: RackmonRequestType req) throws (
    1: fboss.FbossBaseError error,
  );

  map<string, i32> getPlsStatus() throws (1: fboss.FbossBaseError error);
}
