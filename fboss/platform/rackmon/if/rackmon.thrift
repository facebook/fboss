namespace cpp2 facebook.fboss.platform.rackmonsvc
namespace go neteng.fboss.platform.rackmonsvc
namespace py neteng.fboss.platform.rackmon
namespace py3 neteng.fboss.platform.rackmonsvc
namespace py.asyncio neteng.fboss.platform.asyncio.rackmon
namespace rust facebook.fboss.platform.rackmonsvc

include "fboss/agent/if/fboss.thrift"

/*
 * Please refer to below document for the Modbus protocal and terms:
 * https://modbus.org/docs/Modbus_Application_Protocol_V1_1b.pdf
 */

enum RackmonStatusCode {
  SUCCESS = 0,
  ERR_INVALID_ARGS = 1,
  ERR_TIMEOUT = 2,
  ERR_BAD_CRC = 3,
  ERR_IO_FAILURE = 4,
}

enum ModbusDeviceMode {
  ACTIVE = 0,
  DORMANT = 1,
}

enum ModbusDeviceType {
  ORV2_PSU = 0,
}

/*
 * Structure to describe a modbus device (PSU, BBU, etc).
 */
struct ModbusDeviceInfo {
  1: byte devAddress;
  2: ModbusDeviceMode mode;
  3: i32 baudrate;
  4: i32 timeouts;
  5: i32 crcErrors;
  6: i32 miscErrors;
  7: ModbusDeviceType deviceType;
}

/*
 * Request a contiguous block of 16-bit registers from modbus device,
 * such as holding registers, input registers
 */
struct ReadWordRegistersRequest {
  1: byte devAddress;
  2: i32 regAddress;
  3: i32 numRegisters;
  4: optional i32 timeout;
}

struct ReadWordRegistersResponse {
  1: RackmonStatusCode status;
  2: list<i32> regValues;
}

struct WriteSingleRegisterRequest {
  1: byte devAddress;
  2: i32 regAddress;
  3: i32 regValue;
  4: optional i32 timeout;
}

struct PresetMultipleRegistersRequest {
  1: byte devAddress;
  2: i32 regAddress;
  3: list<i32> regValue;
  4: optional i32 timeout;
}

struct FileRecordReq {
  1: i32 fileNum;
  2: i32 recordNum;
  3: i32 dataSize;
}

struct FileRecord {
  1: i32 fileNum;
  2: i32 recordNum;
  3: list<i32> data;
}

struct ReadFileRecordRequest {
  1: byte devAddress;
  2: list<FileRecordReq> records;
  3: optional i32 timeout;
}

struct ReadFileRecordResponse {
  1: RackmonStatusCode status;
  2: list<FileRecord> data;
}

enum RegisterValueType {
  INTEGER = 0,
  STRING = 1,
  FLOAT = 2,
  FLAGS = 3,
  RAW = 4,
}

struct FlagType {
  1: string name;
  2: bool value;
  3: byte bitOffset;
}

union RegisterValue {
  1: i32 intValue;
  2: string strValue;
  3: float floatValue;
  4: list<FlagType> flagsValue;
  5: list<byte> rawValue;
}

struct ModbusRegisterValue {
  1: i32 timestamp;
  2: RegisterValueType type;
  3: RegisterValue value;
}

/*
 * Structure to store cached values of a specific register, and each value
 * is tagged with a timestamp showing when the value is obtained.
 */
struct ModbusRegisterStore {
  1: i32 regAddress;
  2: string name;
  3: list<ModbusRegisterValue> history;
}

/*
 * Structure to store all the cached modbus register values of a specific
 * modbus device.
 */
struct RackmonMonitorData {
  1: ModbusDeviceInfo devInfo;
  2: list<ModbusRegisterStore> regList;
}

enum RackmonControlRequest {
  /* Pause rackmond core loop. */
  PAUSE_RACKMOND = 0,

  /* Resume rackmond core loop. */
  RESUME_RACKMOND = 1,
}

struct PowerPortStatus {
  1: bool powerLost;
  2: bool redundancyLost;
}

struct PowerLossSiren {
  1: PowerPortStatus port1;
  2: PowerPortStatus port2;
  3: PowerPortStatus port3;
}

service RackmonCtrl {
  /*
   * List the summary of all the detected Modbus devices.
   */
  list<ModbusDeviceInfo> listModbusDevices() throws (
    1: fboss.FbossBaseError error,
  );

  /*
   * Modbus function code 0x03, Read Holding Registers.
   */
  ReadWordRegistersResponse readHoldingRegisters(
    1: ReadWordRegistersRequest req,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Modbus function code 0x06, Write Single Register.
   */
  RackmonStatusCode writeSingleRegister(
    1: WriteSingleRegisterRequest req,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Modbus function code 0x10, Write multiple register.
   */
  RackmonStatusCode presetMultipleRegisters(
    1: PresetMultipleRegistersRequest req,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Modbus function code 0x14, read File record.
   */
  ReadFileRecordResponse readFileRecord(1: ReadFileRecordRequest req) throws (
    1: fboss.FbossBaseError error,
  );

  /*
   * Get the pre-fetched register values of all the detected Modbus devices.
   */
  list<RackmonMonitorData> getMonitorData() throws (
    1: fboss.FbossBaseError error,
  );

  /*
   * Send commands to control rackmond's behavior, such as pause/resume
   * rackmond's core loop.
   */
  RackmonStatusCode controlRackmond(1: RackmonControlRequest req) throws (
    1: fboss.FbossBaseError error,
  );

  /*
   * Get the status of 6 Power Loss GPIO pins (3 RS485 ports, 2 pins per port).
   */
  PowerLossSiren getPowerLossSiren() throws (1: fboss.FbossBaseError error);
}
