namespace cpp2 facebook.fboss
namespace go neteng.fboss.sff
namespace php fboss
namespace py neteng.fboss.sff
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.sff

enum SffField {
  // First 10 Fields reserved for fields common between
  // cmis/sff/sff8472
  // Raw Read/Write.
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

  // Fields for entire page reads
  PAGE_LOWER = 11,
  PAGE_UPPER0 = 12,
  PAGE_UPPER3 = 13,
  // Shared QSFP and SFP fields:
  IDENTIFIER = 14, // Type of Transceiver
  STATUS = 15, // Support flags for upper pages
  LOS = 16, // Loss of Signal
  FAULT = 17, // TX Faults
  LOL = 18, // Loss of Lock
  TEMPERATURE_ALARMS = 19,
  VCC_ALARMS = 20, // Voltage
  CHANNEL_RX_PWR_ALARMS = 21,
  CHANNEL_TX_BIAS_ALARMS = 22,
  CHANNEL_TX_PWR_ALARMS = 23,
  TEMPERATURE = 24,
  VCC = 25, // Voltage
  CHANNEL_RX_PWR = 26,
  CHANNEL_TX_BIAS = 27,
  CHANNEL_TX_PWR = 28,
  TX_DISABLE = 29,
  RATE_SELECT_RX = 30,
  RATE_SELECT_TX = 31,
  POWER_CONTROL = 32,
  CDR_CONTROL = 33, // Whether CDR is enabled
  ETHERNET_COMPLIANCE = 34,
  EXTENDED_IDENTIFIER = 35,
  PAGE_SELECT_BYTE = 36,
  LENGTH_SM_KM = 37, // Single mode = , in km
  LENGTH_SM = 38, // Single mode in 100m (not in QSFP)
  LENGTH_OM3 = 39,
  LENGTH_OM2 = 40,
  LENGTH_OM1 = 41,
  LENGTH_COPPER = 42,
  LENGTH_COPPER_DECIMETERS = 43,
  DAC_GAUGE = 44,

  DEVICE_TECHNOLOGY = 45, // Device or cable technology of free side device
  OPTIONS = 46, // Variety of options = , including rate select support
  VENDOR_NAME = 47, // QSFP Vendor Name (ASCII)
  VENDOR_OUI = 48, // QSFP Vendor IEEE company ID
  PART_NUMBER = 49, // Part NUmber provided by QSFP vendor (ASCII)
  REVISION_NUMBER = 50, // Revision number
  VENDOR_SERIAL_NUMBER = 51, // Vendor Serial Number (ASCII)
  MFG_DATE = 52, // Manufacturing date code
  DIAGNOSTIC_MONITORING_TYPE = 53, // Diagnostic monitoring implemented
  TEMPERATURE_THRESH = 54,
  VCC_THRESH = 55,
  RX_PWR_THRESH = 56,
  TX_BIAS_THRESH = 57,
  TX_PWR_THRESH = 58,
  EXTENDED_RATE_COMPLIANCE = 59,
  TX_EQUALIZATION = 60,
  RX_EMPHASIS = 61,
  RX_AMPLITUDE = 62,
  SQUELCH_CONTROL = 63,
  TXRX_OUTPUT_CONTROL = 64,
  EXTENDED_SPECIFICATION_COMPLIANCE = 65,
  FR1_PRBS_SUPPORT = 66,
  TX_CONTROL_SUPPORT = 67,
  RX_CONTROL_SUPPORT = 68,

  PAGE0_CSUM = 69,
  PAGE0_EXTCSUM = 70,
  PAGE1_CSUM = 71,

  // SFP-specific Fields
  /* 0xA0 Address Fields */
  EXT_IDENTIFIER = 72, // Extended type of transceiver
  CONNECTOR_TYPE = 73, // Code for Connector Type
  TRANSCEIVER_CODE = 74, // Code for Electronic or optical capability
  ENCODING_CODE = 75, // High speed Serial encoding algo code
  SIGNALLING_RATE = 76, // nominal signalling rate
  RATE_IDENTIFIER = 77, // type of rate select functionality
  TRANCEIVER_CAPABILITY = 78, // Code for Electronic or optical capability
  WAVELENGTH = 79, // laser wavelength
  CHECK_CODE_BASEID = 80, // Check code for the above fields
  // Extended options
  ENABLED_OPTIONS = 81, // Indicates the optional transceiver signals enabled
  UPPER_BIT_RATE_MARGIN = 82, // upper bit rate margin
  LOWER_BIT_RATE_MARGIN = 83, // lower but rate margin
  ENHANCED_OPTIONS = 84, // Enhanced options implemented
  SFF_COMPLIANCE = 85, // revision number of SFF compliance
  CHECK_CODE_EXTENDED_OPT = 86, // check code for the extended options
  VENDOR_EEPROM = 87,
  /* 0xA2 address Fields */
  /* Diagnostics */
  ALARM_THRESHOLD_VALUES = 88, // diagnostic flag alarm and warning thresh values
  EXTERNAL_CALIBRATION = 89, // diagnostic calibration constants
  CHECK_CODE_DMI = 90, // Check code for base Diagnostic Fields
  DIAGNOSTICS = 91, // Diagnostic Monitor Data
  STATUS_CONTROL = 92, // Optional Status and Control bits
  ALARM_WARN_FLAGS = 93, // Diagnostic alarm and warning flag
  EXTENDED_STATUS_CONTROL = 94, // Extended status and control bytes
  /* General Purpose */
  VENDOR_MEM_ADDRESS = 95, // Vendor Specific memory address
  USER_EEPROM = 96, // User Writable NVM
  VENDOR_CONTROL = 97, // Vendor Specific Control
  /* Miniphoton specific */
  MINIPHOTON_LOOPBACK = 98, // Miniphoton Loopback Mode
  // PRBS Related Fields. These are vendor specific.
  PRBS_PATTERN_CONTROL = 99,
  PRBS_GENERATOR_CONTROL = 100,
  PRBS_CHECKER_CONTROL = 101,
  PRBS_COUNTER_FREEZE = 102,
  PRBS_HOST_BIT_COUNT_LANE0 = 103,
  PRBS_HOST_BIT_COUNT_LANE1 = 104,
  PRBS_HOST_BIT_COUNT_LANE2 = 105,
  PRBS_HOST_BIT_COUNT_LANE3 = 106,
  PRBS_HOST_ERROR_COUNT_LANE0 = 107,
  PRBS_HOST_ERROR_COUNT_LANE1 = 108,
  PRBS_HOST_ERROR_COUNT_LANE2 = 109,
  PRBS_HOST_ERROR_COUNT_LANE3 = 110,
  PRBS_MEDIA_BIT_COUNT = 111,
  PRBS_MEDIA_ERROR_COUNT = 112,
  PRBS_HOST_LOCK = 113,
  PRBS_MEDIA_LOCK = 114,
}
