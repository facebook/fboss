namespace cpp2 rackmonsvc
namespace go rackmonsvc
namespace py rackmonsvc.rackmonsvc

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
  ERR_UNDERFLOW = 5,
  ERR_OVERFLOW = 6,
  ERR_ILLEGAL_FUNCTION = 7,
  ERR_ILLEGAL_DATA_ADDRESS = 8,
  ERR_ILLEGAL_DATA_VALUE = 9,
  ERR_SLAVE_DEVICE_FAILURE = 10,
  ERR_ACKNOWLEDGE = 11,
  ERR_SLAVE_DEVICE_BUSY = 12,
  ERR_NEGATIVE_ACKNOWLEDGE = 13,
  ERR_MEMORY_PARITY_ERROR = 14,
  ERR_UNDEFINED_MODBUS_ERROR = 15,
}

enum ModbusDeviceMode {
  ACTIVE = 0,
  DORMANT = 1,
}

enum ModbusDeviceType {
  ORV2_PSU = 0,
  ORV3_PSU = 1,
  ORV3_RPU = 2,
  ORV3_BBU = 3,
  ORV3_POWER_TETHER = 4,
  ORV3_HPR_PSU = 5,
  ORV3_HPR_BBU = 6,
  ORV3_HPR_PMM_PSU = 7,
  ORV3_HPR_PMM_BBU = 8,
}

/*
 * Structure to describe a modbus device (PSU, BBU, etc).
 */
struct ModbusDeviceInfo {
  1: i16 devAddress;
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
  1: i16 devAddress;
  2: i32 regAddress;
  3: i32 numRegisters;
  4: optional i32 timeout;
}

struct ReadWordRegistersResponse {
  1: RackmonStatusCode status;
  2: list<i32> regValues;
}

struct WriteSingleRegisterRequest {
  1: i16 devAddress;
  2: i32 regAddress;
  3: i32 regValue;
  4: optional i32 timeout;
}

struct PresetMultipleRegistersRequest {
  1: i16 devAddress;
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
  1: i16 devAddress;
  2: list<FileRecordReq> records;
  3: optional i32 timeout;
}

struct ReadFileRecordResponse {
  1: RackmonStatusCode status;
  2: list<FileRecord> data;
}

/*
 * Filter rule to select particular Device(s).
 * Options are to request to filter by either
 * device address or by device type.
 */
union DeviceFilter {
  1: set<i16> addressFilter;
  2: set<ModbusDeviceType> typeFilter;
}

/*
 * Filter rule to select a particular Register(s).
 * Options are to request to filter by either
 * register address or by register name.
 */
union RegisterFilter {
  1: set<i32> addressFilter;
  2: set<string> nameFilter;
}

/*
 * Set of filter rules to allow users to request
 * for only a subset of the data.
 */
struct MonitorDataFilter {
  /*
   * If provided, returns only the devices matching
   * the provided rule, else all devices are returned
   */
  1: optional DeviceFilter deviceFilter;

  /* If provided, returns only the registers (of each
   * device) matching the provided rule, else all
   * registers are returned.
   */
  2: optional RegisterFilter registerFilter;

  /*
   * If true, only the latest value of each register
   * of each device. Otherwise, the default of returning
   * the entire history available for the register.
   */
  3: bool latestValueOnly = false;
}

enum RegisterValueType {
  INTEGER = 0,
  STRING = 1,
  FLOAT = 2,
  FLAGS = 3,
  RAW = 4,
  LONG = 5,
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
  6: i64 longValue;
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

  /* Rescan immediately */
  RESCAN = 2,
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
   * Get the pre-fetched register values of all/subset of the detected Modbus
   * devices. Advanced API to allow to apply filters to select a subset of
   * device, register, value. For more, see documentation on ModbusDataFilter.
   */
  list<RackmonMonitorData> getMonitorDataEx(
    1: MonitorDataFilter filter,
  ) throws (1: fboss.FbossBaseError error);

  /*
   * Reload register matching the provided filter
   */
  void reload(1: MonitorDataFilter filter, 2: bool synchronous = true) throws (
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
