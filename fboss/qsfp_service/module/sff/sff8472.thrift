namespace cpp2 facebook.fboss
namespace go neteng.fboss.sff8472
namespace php fboss
namespace py neteng.fboss.sff8472
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.sff8472

enum Sff8472Field {
  // First 10 Fields reserved for fields common between
  // cmis/sff/sff8472
  // Raw Read/write.
  RAW = 0,
  // Field associated with CDB Command
  CDB_COMMAND = 1,
  // Field associated with FW upgrade
  FW_UPGRADE = 2,
  // Page Change
  PAGE_CHANGE = 3,
  // Management Interface
  MGMT_INTERFACE = 4,
  // Part Number Operations
  PART_NUM = 5,
  // Firmware Version
  FW_VERSION = 6,

  IDENTIFIER = 11, // Type of Transceiver
  ETHERNET_10G_COMPLIANCE_CODE = 12, // 10G Ethernet Compliance codes

  ALARM_WARNING_THRESHOLDS = 13,
  TEMPERATURE = 14,
  VCC = 15,
  CHANNEL_TX_BIAS = 16,
  CHANNEL_TX_PWR = 17,
  CHANNEL_RX_PWR = 18,
  STATUS_AND_CONTROL_BITS = 19,
  ALARM_WARNING_FLAGS = 20,

  EXTENDED_SPEC_COMPLIANCE_CODE = 21,
  VENDOR_NAME = 22,
  VENDOR_OUI = 23,
  VENDOR_PART_NUMBER = 24,
  VENDOR_REVISION_NUMBER = 25,
  VENDOR_SERIAL_NUMBER = 26,
  VENDOR_MFG_DATE = 27,
  DOM_TYPE = 28,

  PAGE_LOWER_A0 = 29,
  PAGE_LOWER_A2 = 30,
}
