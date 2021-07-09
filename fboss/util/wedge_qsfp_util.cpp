// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/util/wedge_qsfp_util.h"

#include "fboss/lib/usb/WedgeI2CBus.h"
#include "fboss/lib/usb/Wedge100I2CBus.h"
#include "fboss/lib/usb/GalaxyI2CBus.h"

#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/module/sff/SffModule.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeQsfp.h"

#include "fboss/qsfp_service/lib/QsfpClient.h"
#include "fboss/qsfp_service/module/cmis/CmisFieldInfo.h"

#include <folly/Conv.h>
#include <folly/Exception.h>
#include <folly/FileUtil.h>
#include <folly/Memory.h>
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <chrono>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sysexits.h>
#include <dirent.h>
#include <fstream>

#include <thread>
#include <vector>
#include <utility>
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace {
const std::map<uint8_t, std::string> kCmisAppNameMapping = {
    {0x10, "100G_CWDM4"},
    {0x18, "200G_FR4"},
};

const std::map<uint8_t, std::string> kCmisModuleStateMapping = {
    {0b001, "LowPower"},
    {0b010, "PoweringUp"},
    {0b011, "Ready"},
    {0b100, "PoweringDown"},
    {0b101, "Fault"},
};

const std::map<uint8_t, std::string> kCmisLaneStateMapping = {
    {0b001, "DEACT"},
    {0b010, "INITL"},
    {0b011, "DEINT"},
    {0b100, "ACTIV"},
    {0b101, "TX_ON"},
    {0b110, "TXOFF"},
    {0b111, "DPINT"},
};

std::string getStateNameString(uint8_t stateCode, const std::map<uint8_t, std::string>& nameMap) {
  std::string stateName = "UNKNOWN";
  if (auto iter = nameMap.find(stateCode);
        iter != nameMap.end()) {
    stateName = iter->second;
  }
  return stateName;
}

constexpr uint8_t kSffLowPowerMode = 0x03;
constexpr uint8_t kSffHighPowerMode = 0x05;
constexpr uint8_t kCMISOffsetAppSelLane1 = 145;
constexpr uint8_t kCMISOffsetStageCtrlSet0 = 143;
// 0x01 overrides low power mode
// 0x04 is an LR4-specific bit that is otherwise reserved

constexpr uint8_t kCmisPowerModeMask = (0b101 << 4);

constexpr int numRetryGetModuleType = 5;

enum VdmDataType : uint8_t {
  VDM_DATA_TYPE_U16 = 1,
  VDM_DATA_TYPE_F16 = 2,
};

enum VdmConfigTypeKey : uint8_t {
  VDM_CONFIG_UNSUPPORTED = 0,
  VDM_CONFIG_LASER_AGE = 1,
  VDM_CONFIG_TEC_CURRENT = 2,
  VDM_CONFIG_LASER_FREQUENCY_ERROR = 3,
  VDM_CONFIG_LASER_TEMPERATURE = 4,
  VDM_CONFIG_ESNR_MEDIA_INPUT = 5,
  VDM_CONFIG_ESNR_HOST_INPUT = 6,
  VDM_CONFIG_PAM4_LTP_MEDIA_INPUT = 7,
  VDM_CONFIG_PAM4_LTP_HOST_INPUT = 8,
  VDM_CONFIG_PRE_FEC_BER_MEDIA_IN_MIN = 9,
  VDM_CONFIG_PRE_FEC_BER_HOST_IN_MIN = 10,
  VDM_CONFIG_PRE_FEC_BER_MEDIA_IN_MAX = 11,
  VDM_CONFIG_PRE_FEC_BER_HOST_IN_MAX = 12,
  VDM_CONFIG_PRE_FEC_BER_MEDIA_IN_AVG = 13,
  VDM_CONFIG_PRE_FEC_BER_HOST_IN_AVG = 14,
  VDM_CONFIG_PRE_FEC_BER_MEDIA_IN_CURR = 15,
  VDM_CONFIG_PRE_FEC_BER_HOST_IN_CURR = 16,
  VDM_CONFIG_ERR_FRAMES_MEDIA_IN_MIN = 17,
  VDM_CONFIG_ERR_FRAMES_HOST_IN_MIN = 18,
  VDM_CONFIG_ERR_FRAMES_MEDIA_IN_MAX = 19,
  VDM_CONFIG_ERR_FRAMES_HOST_IN_MAX = 20,
  VDM_CONFIG_ERR_FRAMES_MEDIA_IN_AVG = 21,
  VDM_CONFIG_ERR_FRAMES_HOST_IN_AVG = 22,
  VDM_CONFIG_ERR_FRAMES_MEDIA_IN_CURR = 23,
  VDM_CONFIG_ERR_FRAMES_HOST_IN_CURR = 24,
};

struct VdmConfigType {
  std::string description;
  VdmDataType dataType;
  float lsbScale;
  std::string unit;
};

std::map<VdmConfigTypeKey, VdmConfigType> vdmConfigTypeMap = {
  {VDM_CONFIG_LASER_AGE, {"Laser age", VDM_DATA_TYPE_U16, 1.0, "%"}},
  // Media side
  {VDM_CONFIG_ESNR_MEDIA_INPUT, {"eSNR Media Input", VDM_DATA_TYPE_U16, 1.0/256.0, "dB"}},
  {VDM_CONFIG_PAM4_LTP_MEDIA_INPUT, {"PAM4 LTP Media Input", VDM_DATA_TYPE_U16, 1.0/256.0, "dB"}},
  {VDM_CONFIG_PRE_FEC_BER_MEDIA_IN_MIN, {"Pre-FEC BER Media Input Minimum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PRE_FEC_BER_MEDIA_IN_MAX, {"Pre-FEC BER Media Input Maximum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PRE_FEC_BER_MEDIA_IN_AVG, {"Pre-FEC BER Media Input Average", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PRE_FEC_BER_MEDIA_IN_CURR, {"Pre-FEC BER Media Input Current", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_MEDIA_IN_MIN, {"Err Frames Media Input Minimum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_MEDIA_IN_MAX, {"Err Frames Media Input Maximum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_MEDIA_IN_AVG, {"Err Frames Media Input Average", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_MEDIA_IN_CURR, {"Err Frames Media Input Current", VDM_DATA_TYPE_F16, 0, ""}},
  // Host side
  {VDM_CONFIG_ESNR_HOST_INPUT, {"eSNR Host Input", VDM_DATA_TYPE_U16, 1.0/256.0, "dB"}},
  {VDM_CONFIG_PAM4_LTP_HOST_INPUT, {"PAM4 LTP Host Input", VDM_DATA_TYPE_U16, 1.0/256.0, "dB"}},
  {VDM_CONFIG_PRE_FEC_BER_HOST_IN_MIN, {"Pre-FEC BER Host Input Minimum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PRE_FEC_BER_HOST_IN_MAX, {"Pre-FEC BER Host Input Maximum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PRE_FEC_BER_HOST_IN_AVG, {"Pre-FEC BER Host Input Average", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PRE_FEC_BER_HOST_IN_CURR, {"Pre-FEC BER Host Input Current", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_HOST_IN_MIN, {"Err Frames Host Input Minimum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_HOST_IN_MAX, {"Err Frames Host Input Maximum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_HOST_IN_AVG, {"Err Frames Host Input Average", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_HOST_IN_CURR, {"Err Frames Host Input Current", VDM_DATA_TYPE_F16, 0, ""}},
};
}

using folly::MutableByteRange;
using folly::StringPiece;
using std::pair;
using std::make_pair;
using std::chrono::seconds;
using std::chrono::steady_clock;
using apache::thrift::can_throw;

// We can check on the hardware type:

const char *chipCheckPath = "/sys/bus/pci/devices/0000:01:00.0/device";
const char *trident2 = "0xb850\n";  // Note expected carriage return
static constexpr uint16_t hexBase = 16;
static constexpr uint16_t decimalBase = 10;
static constexpr uint16_t eePromDefault = 255;
static constexpr uint16_t maxGauge = 30;

DEFINE_bool(clear_low_power, false,
            "Allow the QSFP to use higher power; needed for LR4 optics");
DEFINE_bool(set_low_power, false,
            "Force the QSFP to limit power usage; Only useful for testing");
DEFINE_bool(tx_disable, false, "Set the TX disable bits");
DEFINE_bool(tx_enable, false, "Clear the TX disable bits");
DEFINE_bool(set_40g, false, "Rate select 40G, for CWDM4 modules.");
DEFINE_bool(set_100g, false, "Rate select 100G, for CWDM4 modules.");
DEFINE_int32(app_sel, 0,
            "Select application, for CMIS modules. In 200G modules 1 is for 200G and 2 is for 100G");
DEFINE_bool(cdr_enable, false, "Set the CDR bits if transceiver supports it");
DEFINE_bool(cdr_disable, false,
    "Clear the CDR bits if transceiver supports it");
DEFINE_int32(open_timeout, 30, "Number of seconds to wait to open bus");
DEFINE_bool(direct_i2c, false,
    "Read Transceiver info from i2c bus instead of qsfp_service");
DEFINE_bool(qsfp_hard_reset, false, "Issue a hard reset to port QSFP");
DEFINE_bool(electrical_loopback, false,
            "Set the module to be electrical loopback, only for Miniphoton");
DEFINE_bool(optical_loopback, false,
            "Set the module to be optical loopback, only for Miniphoton");
DEFINE_bool(clear_loopback, false,
            "Clear the module loopback bits, only for Miniphoton");
DEFINE_bool(read_reg, false, "Read a register, use with --offset and optionally --length");
DEFINE_bool(write_reg, false, "Write a register, use with --offset and --data");
DEFINE_int32(offset, -1, "The offset of register to read/write (0..255)");
DEFINE_int32(data, 0, "The byte to write to the register, use with --offset");
DEFINE_int32(length, 1, "The number of bytes to read from the register (1..128), use with --offset");
DEFINE_int32(page, -1, "The page to use when doing transceiver register access");
DEFINE_int32(pause_remediation, 0,
    "Number of seconds to prevent qsfp_service from doing remediation to modules");
DEFINE_bool(get_remediation_until_time, false,
    "Get the local time that current remediationUntil time sets to");
DEFINE_bool(update_module_firmware, false,
            "Update firmware for module, use with --firmware_filename");
DEFINE_string(firmware_filename, "",
            "Module firmware filename along with path");
DEFINE_uint32(msa_password, 0x00001011, "MSA password for module privilige operation");
DEFINE_uint32(image_header_len, 0, "Firmware image header length");
DEFINE_bool(get_module_fw_info, false, "Get the module firmware info for list of ports, use with portA and portB");
DEFINE_bool(cdb_command, false, "Read from CDB block, use with --command_code");
DEFINE_int32(command_code, -1, "CDB command code (16 bit)");
DEFINE_string(cdb_payload, "", "CDB command LPL payload (list of bytes)");
DEFINE_bool(update_bulk_module_fw, false,
            "Update firmware for module, use with --firmware_filename, --module_type, --fw_version, --port_range");
DEFINE_string(module_type, "", "specify the module type, ie: finisar-200g");
DEFINE_string(fw_version, "", "specify the firmware version, ie: 7.8 or ca.f8");
DEFINE_string(port_range, "", "specify the port range, ie: 1,3,5-8");
DEFINE_bool(dsp_image, false, "if this is a DSP firmware image");
DEFINE_bool(
    client_parser,
    false,
    "Used to print DOM data that is parsed by the client as opposed to parsed by the service(which is the default)");
DEFINE_bool(verbose, false, "Print more details");
DEFINE_bool(list_commands, false, "Print the list of commands");
DEFINE_bool(vdm_info, false, "get the VDM monitoring info");

namespace {
struct ModulePartInfo_s {
  std::array<uint8_t, 16> partNo;
  uint32_t headerLen;
};
struct ModulePartInfo_s modulePartInfo[] = {
  // Finisar 200G module info
  {{'F','T','C','C','1','1','1','2','E','1','P','L','L','-','F','B'}, 64},
  // Finisar 400G module info
  {{'F','T','C','D','4','3','1','3','E','2','P','C','L',0x20,0x20,0x20}, 64},
  // Innolight 200G module info
  {{'T','-','F','X','4','F','N','T','-','H','F','B',0x20,0x20,0x20,0x20}, 48},
  // Innolight 400G module info
  {{'T','-','D','Q','4','C','N','T','-','N','F','B',0x20,0x20,0x20,0x20}, 48}
};
constexpr uint8_t kNumModuleInfo = sizeof(modulePartInfo)/sizeof(struct ModulePartInfo_s);
};

namespace facebook::fboss {

// Forward declaration of utility functions for firmware upgrade
std::vector<unsigned int> getUpgradeModList(
      TransceiverI2CApi* bus,
      std::vector<unsigned int> portlist,
      std::string moduleType,
      std::string fwVer);

void fwUpgradeThreadHandler(
    TransceiverI2CApi* bus,
    std::vector<unsigned int> modlist,
    std::string firmwareFilename,
    uint32_t imageHdrLen);

std::ostream& operator<<(std::ostream& os, const FlagCommand& cmd) {
  gflags::CommandLineFlagInfo flagInfo;

  os << "Command:\n"
   << gflags::DescribeOneFlag(gflags::GetCommandLineFlagInfoOrDie(cmd.command.c_str()));
  os << "Flags:\n";
  for (auto flag: cmd.flags) {
    os << gflags::DescribeOneFlag(gflags::GetCommandLineFlagInfoOrDie(flag.c_str()));
  }

  return os;
}

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> getQsfpClient(folly::EventBase& evb) {
  return std::move(QsfpClient::createClient(&evb)).getVia(&evb);
}

/*
 * This function returns the module type whether it is CMIS or SFF type
 * by reading the register 0 from module
 */
TransceiverManagementInterface getModuleType(
  TransceiverI2CApi* bus, unsigned int port) {
  uint8_t moduleId;

  // Get the module id to differentiate between CMIS (0x1e) and SFF
  for (auto retry = 0; retry < numRetryGetModuleType; retry++) {
    try {
      bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 0, 1, &moduleId);
    } catch (const I2cError& ex) {
      fprintf(stderr, "QSFP %d: not present or read error, retrying...\n", port);
    }
  }

  if (moduleId == static_cast<uint8_t>(TransceiverModuleIdentifier::QSFP_PLUS_CMIS) ||
      moduleId == static_cast<uint8_t>(TransceiverModuleIdentifier::QSFP_DD)) {
    return TransceiverManagementInterface::CMIS;
  } else if (moduleId == static_cast<uint8_t>(TransceiverModuleIdentifier::QSFP_PLUS) ||
      moduleId == static_cast<uint8_t>(TransceiverModuleIdentifier::QSFP) ||
      moduleId == static_cast<uint8_t>(TransceiverModuleIdentifier::QSFP28)) {
    return TransceiverManagementInterface::SFF;
  } else {
    return TransceiverManagementInterface::NONE;
  }
}

bool flipModuleUpperPage(TransceiverI2CApi* bus, unsigned int port, uint8_t page) {
  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 127, sizeof(page), &page);
  } catch (const I2cError& ex) {
    fprintf(stderr, "QSFP %d: Fail to flip module upper page\n", port);
    return false;
  }
  return true;
}

bool overrideLowPower(
    TransceiverI2CApi* bus,
    unsigned int port,
    bool lowPower) {
  std::array<uint8_t, 1> buf;
  try {
    // First figure out whether this module follows CMIS or Sff8636.
    auto managementInterface = getModuleType(bus, port);
    if (managementInterface == TransceiverManagementInterface::CMIS) {
      bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 26, 1, buf.data());
      lowPower ? buf[0] |= kCmisPowerModeMask : buf[0] &= ~kCmisPowerModeMask;
      bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 26, 1, buf.data());
    } else if (managementInterface == TransceiverManagementInterface::SFF) {
      buf[0] = {lowPower ? kSffLowPowerMode : kSffHighPowerMode};
      bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 93, 1, buf.data());
    } else {
      fprintf(stderr, "QSFP %d: Unrecognizable management interface\n", port);
      return false;
    }
  } catch (const I2cError& ex) {
    // This generally means the QSFP module is not present.
    fprintf(stderr, "QSFP %d: not present or unwritable\n", port);
    return false;
  }
  return true;
}

bool setCdr(TransceiverI2CApi* bus, unsigned int port, uint8_t value) {
  // 0xff to enable
  // 0x00 to disable

  // Check if CDR is supported
  uint8_t supported[1];
  try {
    // ensure we have page0 selected
    uint8_t page0 = 0;
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP,
                     127, 1, &page0);

    bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 129,
                      1, supported);
  } catch (const I2cError& ex) {
    fprintf(stderr, "Port %d: Unable to determine whether CDR supported: %s\n",
            port, ex.what());
    return false;
  }
  // If 2nd and 3rd bits are set, CDR is supported.
  if ((supported[0] & 0xC) != 0xC) {
    fprintf(stderr, "CDR unsupported by this device, doing nothing");
    return false;
  }

  // Even if CDR isn't supported for one of RX and TX, set the whole
  // byte anyway
  uint8_t buf[1] = {value};
  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 98, 1, buf);
  } catch (const I2cError& ex) {
    fprintf(stderr, "QSFP %d: Failed to set CDR\n", port);
    return false;
  }
  return true;
}

bool rateSelect(TransceiverI2CApi* bus, unsigned int port, uint8_t value) {
  // If v1 is used, both at 10, if v2
  // 0b10 - 25G channels
  // 0b00 - 10G channels
  uint8_t version[1];
  try {
    // ensure we have page0 selected
    uint8_t page0 = 0;
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP,
                     127, 1, &page0);

    bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 141,
                      1, version);
  } catch (const I2cError& ex) {
    fprintf(stderr,
        "Port %d: Unable to determine rate select version in use, defaulting \
        to V1\n",
        port);
    version[0] = 0b01;
  }

  uint8_t buf[1];
  if (version[0] & 1) {
    buf[0] = 0b10;
  } else {
    buf[0] = value;
  }

  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 87, 1, buf);
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 88, 1, buf);
  } catch (const I2cError& ex) {
    // This generally means the QSFP module is not present.
    fprintf(stderr, "QSFP %d: not present or unwritable\n", port);
    return false;
  }
  return true;
}

bool appSel(TransceiverI2CApi* bus, unsigned int port, uint8_t value) {
  // This function is for CMIS module to change application.
  // Application code is the upper four bit of the field.
  uint8_t applicationCode = value << 4;
  try {
    // Flip to page 0x10 to get prepared.
    uint8_t page = 0x10;
    flipModuleUpperPage(bus, port, page);

    for (int channel = 0; channel < 4; channel++) {
      bus->moduleWrite(
          port,
          TransceiverI2CApi::ADDR_QSFP,
          kCMISOffsetAppSelLane1 + channel,
          sizeof(applicationCode),
          &applicationCode);
    }

    uint8_t applySet0 = 0x0f;
    bus->moduleWrite(
        port,
        TransceiverI2CApi::ADDR_QSFP,
        kCMISOffsetStageCtrlSet0,
        sizeof(applySet0),
        &applySet0);
  } catch (const I2cError& ex) {
    // This generally means the QSFP module is not present.
    fprintf(stderr, "QSFP %d: fail to change application\n", port);
    return false;
  }
  return true;
}

/*
 * This function disables the optics lane TX which brings down the port. The
 * TX Disable will cause LOS at the link partner and Remote Fault at this end.
 */
bool setTxDisable(TransceiverI2CApi* bus, unsigned int port, bool disable) {
  std::array<uint8_t, 1> buf;

  // Get module type CMIS or SFF
  auto moduleType = getModuleType(bus, port);

  if (moduleType == TransceiverManagementInterface::SFF) {
    // For SFF the value 0xf disables all 4 lanes and 0x0 enables it back
    buf[0] = disable ? 0xf : 0x0;

    // For SFF module, the page 0 reg 86 controls TX_DISABLE for 4 lanes
    try {
      bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 86, 1, &buf[0]);
    } catch (const I2cError& ex) {
      fprintf(stderr, "QSFP %d: unwritable or write error\n", port);
      return false;
    }
  } else if (moduleType == TransceiverManagementInterface::CMIS) {
    // For CMIS module, the page 0x10 reg 130 controls TX_DISABLE for 8 lanes
    uint8_t savedPage, moduleControlPage=0x10;

    // For CMIS the value 0xff disables all 8 lanes and 0x0 enables it back
    buf[0] = disable ? 0xff : 0x0;

    try {
      // Save current page
      bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 127, 1, &savedPage);
      // Write page 10 reg 130
      bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 127, 1, &moduleControlPage);
      bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 130, 1, &buf[0]);
      // Restore current page
      bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 127, 1, &savedPage);
    } catch (const I2cError& ex) {
      fprintf(stderr, "QSFP %d: read/write error\n", port);
      return false;
    }
  } else {
    fprintf(stderr, "QSFP %d: Unrecognized transceiver management interface.\n", port);
    return false;
  }

  return true;
}

std::vector<int32_t> zeroBasedPortIds(std::vector<unsigned int>& ports) {
  std::vector<int32_t> idx;
  for (auto port : ports) {
    // Direct I2C bus starts from 1 instead of 0, however qsfp_service
    // index starts from 0. So here we try to comply to match that
    // behavior.
    idx.push_back(port - 1);
  }
  return idx;
}

void displayReadRegData(const uint8_t* buf, int offset, int length) {
  // Print the read registers
  // Print 16 bytes in a line with offset at start and extra gap after 8 bytes
  for (int i = 0; i < length; i++) {
    if (i % 16 == 0) {
      // New line after 16 bytes (except the first line)
      if (i != 0) {
        printf("\n");
      }
      // 2 byte offset at start of every line
      printf("%04x: ", offset + i);
    } else if (i % 8 == 0) {
      // Extra gap after 8 bytes in a line
      printf(" ");
    }
    printf("%02x ", buf[i]);
  }
  printf("\n");
}

void doReadRegViaService(
    std::vector<int32_t>& ports,
    int offset,
    int length,
    int page,
    folly::EventBase& evb) {
  auto client = getQsfpClient(evb);
  ReadRequest request;
  TransceiverIOParameters param;
  request.ids_ref() = ports;
  param.offset_ref() = offset;
  param.length_ref() = length;
  if (page != -1) {
    param.page_ref() = page;
  }
  request.parameter_ref() = param;
  std::map<int32_t, ReadResponse> response;

  try {
    client->sync_readTransceiverRegister(response, request);
    for (const auto& iterator : response) {
      printf("Port Id : %d\n", iterator.first + 1);
      auto data = iterator.second.data_ref()->data();
      if (iterator.second.data_ref()->length() < length) {
        fprintf(stderr, "QSFP %d: fail to read module\n", iterator.first + 1);
      } else {
        displayReadRegData(data, offset, iterator.second.data_ref()->length());
      }
    }
  } catch (const std::exception& ex) {
    fprintf(
        stderr, "error reading register from qsfp_service: %s\n", ex.what());
  }
}

void doReadRegDirect(TransceiverI2CApi* bus, unsigned int port, int offset, int length) {
  std::array<uint8_t, 128> buf;
  try {
    bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, offset, length, buf.data());
  } catch (const I2cError& ex) {
    fprintf(stderr, "QSFP %d: fail to read module\n", port);
    return;
  }
  displayReadRegData(buf.data(), offset, length);
}

int doReadReg(
    TransceiverI2CApi* bus,
    std::vector<unsigned int>& ports,
    int offset,
    int length,
    int page,
    folly::EventBase& evb) {
  if (offset == -1) {
    fprintf(
        stderr,
        "QSFP %s: Fail to read register. Specify offset using --offset\n",
        folly::join(",", ports).c_str());
    return EX_SOFTWARE;
  }
  if (length > 128) {
    fprintf(
        stderr,
        "QSFP %s: Fail to read register. The --length value should be between 1 to 128\n",
        folly::join(",", ports).c_str());
    return EX_SOFTWARE;
  }

  if (FLAGS_direct_i2c) {
    for (unsigned int portNum : ports) {
      printf("Port Id : %d\n", portNum);
      doReadRegDirect(bus, portNum, FLAGS_offset, FLAGS_length);
    }
  } else {
    // Release the bus access for QSFP service
    bus->close();
    std::vector<int32_t> idx = zeroBasedPortIds(ports);
    doReadRegViaService(idx, offset, length, page, evb);
  }
  return EX_OK;
}

void doWriteRegDirect(TransceiverI2CApi* bus, unsigned int port, int offset, uint8_t value) {
  uint8_t buf[1] = {value};
  try {
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, offset, 1, buf);
  } catch (const I2cError& ex) {
    fprintf(stderr, "QSFP %d: not present or unwritable\n", port);
    return;
  }
  printf("QSFP %d: successfully write 0x%02x to %d.\n", port, value, offset);
}

void doWriteRegViaService(
    std::vector<int32_t>& ports,
    int offset,
    int page,
    uint8_t value,
    folly::EventBase& evb) {
  auto client = getQsfpClient(evb);
  WriteRequest request;
  TransceiverIOParameters param;
  request.ids_ref() = ports;
  param.offset_ref() = offset;
  if (page != -1) {
    param.page_ref() = page;
  }
  request.parameter_ref() = param;
  request.data_ref() = value;
  std::map<int32_t, WriteResponse> response;

  try {
    client->sync_writeTransceiverRegister(response, request);
    for (const auto& iterator : response) {
      if (*(response[iterator.first].success_ref())) {
        printf(
            "QSFP %d: successfully write 0x%02x to %d.\n",
            iterator.first + 1,
            value,
            offset);
      } else {
        fprintf(stderr, "QSFP %d: not present or unwritable\n", iterator.first + 1);
      }
    }
  } catch (const std::exception& ex) {
    fprintf(stderr, "error writing register via qsfp_service: %s\n", ex.what());
    return;
  }
}

int doWriteReg(
    TransceiverI2CApi* bus,
    std::vector<unsigned int>& ports,
    int offset,
    int page,
    uint8_t data,
    folly::EventBase& evb) {
  if (offset == -1) {
    fprintf(
        stderr,
        "QSFP %s: Fail to write register. Specify offset using --offset\n",
        folly::join(",", ports).c_str());
    return EX_SOFTWARE;
  }
  if (FLAGS_direct_i2c) {
    for (unsigned int portNum : ports) {
      doWriteRegDirect(bus, portNum, offset, data);
    }
  } else {
    // Release the bus access for QSFP service
    bus->close();
    std::vector<int32_t> idx = zeroBasedPortIds(ports);
    doWriteRegViaService(idx, offset, page, data, evb);
  }
  return EX_OK;
}

std::map<int32_t, DOMDataUnion> fetchDataFromQsfpService(
    const std::vector<int32_t>& ports, folly::EventBase& evb) {
  auto client = getQsfpClient(evb);

  std::map<int32_t, TransceiverInfo> qsfpInfoMap;

  client->sync_getTransceiverInfo(qsfpInfoMap, ports);

  std::vector<int32_t> presentPorts;
  for(auto& qsfpInfo : qsfpInfoMap) {
    if (*qsfpInfo.second.present_ref()) {
      presentPorts.push_back(qsfpInfo.first);
    }
  }

  std::map<int32_t, DOMDataUnion> domDataUnionMap;

  if(!presentPorts.empty()) {
    client->sync_getTransceiverDOMDataUnion(domDataUnionMap, presentPorts);
  }

  return domDataUnionMap;
}

std::map<int32_t, TransceiverInfo> fetchInfoFromQsfpService(
    const std::vector<int32_t>& ports,
    folly::EventBase& evb) {
  auto client = getQsfpClient(evb);

  std::map<int32_t, TransceiverInfo> qsfpInfoMap;

  client->sync_getTransceiverInfo(qsfpInfoMap, ports);
  return qsfpInfoMap;
}

DOMDataUnion fetchDataFromLocalI2CBus(TransceiverI2CApi* bus, unsigned int port) {
  // port is 1 based and WedgeQsfp is 0 based.
  auto qsfpImpl = std::make_unique<WedgeQsfp>(port - 1, bus);
  auto mgmtInterface = qsfpImpl->getTransceiverManagementInterface();
  if (mgmtInterface == TransceiverManagementInterface::CMIS)
    {
      auto cmisModule =
          std::make_unique<CmisModule>(
              nullptr,
              std::move(qsfpImpl),
              1);
      try {
        cmisModule->refresh();
      } catch (FbossError& e) {
        printf("refresh() FbossError for port %d\n", port);
      } catch (I2cError& e) {
        printf("refresh() YampI2cError for port %d\n", port);
      }
      return cmisModule->getDOMDataUnion();
    } else if (mgmtInterface == TransceiverManagementInterface::SFF)
    {
      auto sffModule =
          std::make_unique<SffModule>(
              nullptr,
              std::move(qsfpImpl),
              1);
      sffModule->refresh();
      return sffModule->getDOMDataUnion();
    } else {
      throw std::runtime_error(folly::sformat(
          "Unknown transceiver management interface: {}.",
          static_cast<int>(mgmtInterface)));
    }
}

void printPortSummary(TransceiverI2CApi*) {
  // TODO: Implement code for showing a summary of all ports.
  // At the moment I haven't tested this since my test switch has some
  // 3M modules plugged in that hang up the bus sometimes when accessed by our
  // CP2112 switch.  I'll implement this in a subsequent diff.
  fprintf(stderr, "Please specify a port number\n");
  exit(1);
}

StringPiece sfpString(const uint8_t* buf, size_t offset, size_t len) {
  const uint8_t* start = buf + offset;
  while (len > 0 && start[len - 1] == ' ') {
    --len;
  }
  return StringPiece(reinterpret_cast<const char*>(start), len);
}

void printThresholds(
    const std::string& name,
    const uint8_t* data,
    std::function<double(uint16_t)> conversionCb) {
  std::array<std::array<uint8_t, 2>, 4> byteOffset{{
      {0, 1},
      {2, 3},
      {4, 5},
      {6, 7},
  }};

  printf("\n");
  const std::array<std::string, 4> thresholds{
      "High Alarm", "Low Alarm", "High Warning", "Low Warning"};

  for (auto row = 0; row < 4; row++) {
    uint16_t u16 = 0;
    for (auto col = 0; col < 2; col++) {
      u16 = (u16 << 8 | data[byteOffset[row][col]]);
    }
    printf(
        "%10s %12s %f\n",
        name.c_str(),
        thresholds[row].c_str(),
        conversionCb(u16));
  }
}

std::string getLocalTime(std::time_t t) {
  struct tm localtime_result;
  localtime_r(&t, &localtime_result);
  std::array<char, 26> buf;
  return asctime_r(&localtime_result,buf.data());
}

void doGetRemediationUntilTime(folly::EventBase& evb) {
  auto client = getQsfpClient(evb);
  auto remediationUntilEpoch = client->sync_getRemediationUntilTime();
  printf(
      "RemediationUntil time is set to : %s",
      getLocalTime(remediationUntilEpoch).c_str());
}

void printChannelMonitor(unsigned int index,
                         const uint8_t* buf,
                         unsigned int rxMSB,
                         unsigned int rxLSB,
                         unsigned int txBiasMSB,
                         unsigned int txBiasLSB,
                         unsigned int txPowerMSB,
                         unsigned int txPowerLSB,
                         std::optional<double> rxSNR = std::nullopt) {
  uint16_t rxValue = (buf[rxMSB] << 8) | buf[rxLSB];
  uint16_t txPowerValue = (buf[txPowerMSB] << 8) | buf[txPowerLSB];
  uint16_t txBiasValue = (buf[txBiasMSB] << 8) | buf[txBiasLSB];

  // RX power ranges from 0mW to 6.5535mW
  double rxPower = 0.0001 * rxValue;

  // TX power ranges from 0mW to 6.5535mW
  double txPower = 0.0001 * txPowerValue;

  // TX bias ranges from 0mA to 131mA
  double txBias = (131.0 * txBiasValue) / 65535;

  if (rxSNR) {
    printf("    Channel %d:   %12fmW  %12fmW  %12fmA  %12f\n",
           index, rxPower, txPower, txBias, rxSNR.value());
  } else {
    printf("    Channel %d:   %12fmW  %12fmW  %12fmA  %12s\n",
           index, rxPower, txPower, txBias, "N/A");
  }
}

uint8_t getSensorMonitors(const Sensor& sensor) {
  uint8_t value = 0;
  if (auto flags = sensor.flags_ref()) {
    value = (*(flags->alarm_ref()->high_ref()) << 3) |
        (*(flags->alarm_ref()->low_ref()) << 2) |
        (*(flags->warn_ref()->high_ref()) << 1) |
        (*(flags->warn_ref()->low_ref()));
  }
  return value;
}

void printLaneLine(const char* detail, unsigned int numChannels) {
  if (numChannels == 4) {
    printf(
        "%-22s     %-12s %-12s %-12s %-12s",
        detail,
        "Lane 1",
        "Lane 2",
        "Lane 3",
        "Lane 4");
  } else if (numChannels == 1) {
        printf(
        "%-22s     %-12s",
        detail,
        "Lane 1");
  } else {
    printf(
        "%-22s     %-12s %-12s %-12s %-12s %-12s %-12s %-12s %-12s",
        detail,
        "Lane 1",
        "Lane 2",
        "Lane 3",
        "Lane 4",
        "Lane 5",
        "Lane 6",
        "Lane 7",
        "Lane 8");
  }
}

void printGlobalInterruptFlags(const GlobalSensors& sensors) {
  printf("  Global Interrupt Flags: \n");
  printf("    Temp: 0x%02x\n", getSensorMonitors(*(sensors.temp_ref())));
  printf("    Vcc: 0x%02x\n", getSensorMonitors(*(sensors.vcc_ref())));
}

void printChannelInterruptFlags(const std::vector<Channel>& channels) {
  int numChannels = channels.size();
  printLaneLine("  Lane Interrupts: ", numChannels);

  auto getHighAlarm = [](const Sensor& sensor){
    return *(sensor.flags_ref()->alarm_ref()->high_ref());
  };
  auto getHighWarn = [](const Sensor& sensor) {
    return *(sensor.flags_ref()->warn_ref()->high_ref());
  };
  auto getLowAlarm = [](const Sensor& sensor) {
    return *(sensor.flags_ref()->alarm_ref()->low_ref());
  };
  auto getLowWarn = [](const Sensor& sensor) {
    return *(sensor.flags_ref()->warn_ref()->low_ref());
  };

  printf("\n    %-22s", "Tx Pwr High Alarm");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getHighAlarm(*(channel.sensors_ref()->txPwr_ref())));
  }
  printf("\n    %-22s", "Tx Pwr High Warn");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getHighWarn(*(channel.sensors_ref()->txPwr_ref())));
  }
  printf("\n    %-22s", "Tx Pwr Low Alarm");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getLowAlarm(*(channel.sensors_ref()->txPwr_ref())));
  }
  printf("\n    %-22s", "Tx Pwr Low Warn");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getLowWarn(*(channel.sensors_ref()->txPwr_ref())));
  }

  printf("\n    %-22s", "Tx Bias High Alarm");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getHighAlarm(*(channel.sensors_ref()->txBias_ref())));
  }
  printf("\n    %-22s", "Tx Bias High Warn");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getHighWarn(*(channel.sensors_ref()->txBias_ref())));
  }
  printf("\n    %-22s", "Tx Bias Low Alarm");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getLowAlarm(*(channel.sensors_ref()->txBias_ref())));
  }
  printf("\n    %-22s", "Tx Bias Low Warn");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getLowWarn(*(channel.sensors_ref()->txBias_ref())));
  }

  printf("\n    %-22s", "Rx Pwr High Alarm");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getHighAlarm(*(channel.sensors_ref()->rxPwr_ref())));
  }
  printf("\n    %-22s", "Rx Pwr High Warn");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getHighWarn(*(channel.sensors_ref()->rxPwr_ref())));
  }
  printf("\n    %-22s", "Rx Pwr Low Alarm");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getLowAlarm(*(channel.sensors_ref()->rxPwr_ref())));
  }
  printf("\n    %-22s", "Rx Pwr Low Warn");
  for (const auto& channel : channels) {
    printf(
        " %-12d", getLowWarn(*(channel.sensors_ref()->rxPwr_ref())));
  }

  printf("\n");
}

void printHostLaneSignals(const std::vector<HostLaneSignals>& signals) {
  unsigned int numLanes = signals.size();
  if (numLanes == 0) {
    return;
  }
  printLaneLine("  Host Lane Signals: ", numLanes);
  // Assumption: If a signal is valid for first lane, it is also valid for other
  // lanes.
  if (signals[0].dataPathDeInit_ref()) {
    printf("\n    %-22s", "Datapath de-init");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.dataPathDeInit_ref()));
    }
  }
  if (signals[0].cmisLaneState_ref()) {
    printf("\n    %-22s", "Lane state");
    for (const auto& signal : signals) {
      printf(
          " %-12s",
          apache::thrift::util::enumNameSafe(
              *(signal.cmisLaneState_ref()))
              .c_str());
    }
  }
  printf("\n");
}

void printMediaLaneSignals(const std::vector<MediaLaneSignals>& signals) {
  unsigned int numLanes = signals.size();
  if (numLanes == 0) {
    return;
  }
  printLaneLine("  Media Lane Signals: ", numLanes);
  // Assumption: If a signal is valid for first lane, it is also valid for other
  // lanes.
  if (signals[0].txLos_ref()) {
    printf("\n    %-22s", "Tx LOS");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.txLos_ref()));
    }
  }
  if (signals[0].txLol_ref()) {
    printf("\n    %-22s", "Tx LOL");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.txLol_ref()));
    }
  }
  if (signals[0].rxLos_ref()) {
    printf("\n    %-22s", "Rx LOS");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.rxLos_ref()));
    }
  }
  if (signals[0].rxLol_ref()) {
    printf("\n    %-22s", "Rx LOL");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.rxLol_ref()));
    }
  }
  if (signals[0].txFault_ref()) {
    printf("\n    %-22s", "Tx Fault");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.txFault_ref()));
    }
  }
  if (signals[0].txAdaptEqFault_ref()) {
    printf("\n    %-22s", "Tx Adaptive Eq Fault");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.txAdaptEqFault_ref()));
    }
  }
  printf("\n");
}

void printHostLaneSettings(const std::vector<HostLaneSettings>& settings) {
  unsigned int numLanes = settings.size();
  if (numLanes == 0) {
    return;
  }
  printLaneLine("  Host Lane Settings:", numLanes);
  if (settings[0].txInputEqualization_ref()) {
    printf("\n    %-22s", "Tx Input Equalization");
    for (const auto& setting : settings) {
      printf(" 0x%-10x", *(setting.txInputEqualization_ref()));
    }
  }
  if (settings[0].rxOutputEmphasis_ref()) {
    printf("\n    %-22s", "Rx Output Emphasis");
    for (const auto& setting : settings) {
      printf(" 0x%-10x", *(setting.rxOutputEmphasis_ref()));
    }
  }
  if (settings[0].rxOutputPreCursor_ref()) {
    printf("\n    %-22s", "Rx Out Precursor");
    for (const auto& setting : settings) {
      auto pre = *(setting.rxOutputPreCursor_ref());
      printf(" %-12d", pre);
    }
  }
  if (settings[0].rxOutputPostCursor_ref()) {
    printf("\n    %-22s", "Rx Out Postcursor");
    for (const auto& setting : settings) {
      auto post = *(setting.rxOutputPostCursor_ref());
      printf(" %-12d", post);
    }
  }
  if (settings[0].rxOutputAmplitude_ref()) {
    printf("\n    %-22s", "Rx Out Amplitude");
    for (const auto& setting : settings) {
      auto amp = *(setting.rxOutputAmplitude_ref());
      printf(" %-12d", amp);
    }
  }
  if (settings[0].rxOutput_ref()) {
    printf("\n    %-22s", "Rx Output Disable");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.rxOutput_ref()));
    }
  }
  if (settings[0].rxSquelch_ref()) {
    printf("\n    %-22s", "Rx Squelch Disable");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.rxSquelch_ref()));
    }
  }
  printf("\n");
}

void printMediaLaneSettings(const std::vector<MediaLaneSettings>& settings) {
  unsigned int numLanes = settings.size();
  if (numLanes == 0) {
    return;
  }
  printLaneLine("  Media Lane Settings:", numLanes);
  if (settings[0].txDisable_ref()) {
    printf("\n    %-22s", "Tx Disable");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.txDisable_ref()));
    }
  }
  if (settings[0].txSquelch_ref()) {
    printf("\n    %-22s", "Tx Squelch Disable");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.txSquelch_ref()));
    }
  }
  if (settings[0].txAdaptiveEqControl_ref()) {
    printf("\n    %-22s", "Tx Adaptive Eq Ctrl");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.txAdaptiveEqControl_ref()));
    }
  }
  if (settings[0].txSquelchForce_ref()) {
    printf("\n    %-22s", "Tx Forced Squelch");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.txSquelchForce_ref()));
    }
  }
  printf("\n");
}

void printGlobalDomMonitors(const GlobalSensors& sensors) {
  printf("  Global DOM Monitors:\n");
  printf("    Temperature: %.2f C\n", *(sensors.temp_ref()->value_ref()));
  printf("    Supply Voltage: %.2f V\n", *(sensors.vcc_ref()->value_ref()));
}

void printLaneDomMonitors(const std::vector<Channel>& channels) {
  int numChannels = channels.size();
  printLaneLine("  Lane Dom Monitors:", numChannels);

  printf("\n    %-22s", "Tx Pwr (mW)");
  for (const auto& channel : channels) {
    printf(
        " %-12.2f", *(channel.sensors_ref()->txPwr_ref()->value_ref()));
  }
  if (channels[0].sensors_ref()->txPwrdBm_ref()) {
    printf("\n    %-22s", "Tx Pwr (dBm)");
    for (const auto& channel : channels) {
      printf(
          " %-12.2f",
          *(channel.sensors_ref()->txPwrdBm_ref()->value_ref()));
    }
  }
  printf("\n    %-22s", "Rx Pwr (mW)");
  for (const auto& channel : channels) {
    printf(
        " %-12.2f", *(channel.sensors_ref()->rxPwr_ref()->value_ref()));
  }
  if (channels[0].sensors_ref()->rxPwrdBm_ref()) {
    printf("\n    %-22s", "Rx Pwr (dBm)");
    for (const auto& channel : channels) {
      printf(
          " %-12.2f",
          *(channel.sensors_ref()->rxPwrdBm_ref()->value_ref()));
    }
  }
  printf("\n    %-22s", "Tx Bias (mA)");
  for (const auto& channel : channels) {
    printf(
        " %-12.2f", *(channel.sensors_ref()->txBias_ref()->value_ref()));
  }
  if (auto rxSnrCh0 = channels[0].sensors_ref()->rxSnr_ref()) {
    printf("\n    %-22s", "Rx SNR");
    for (const auto& channel : channels) {
      if (auto snr = channel.sensors_ref()->rxSnr_ref()) {
        printf(
          " %-12.2f", *((*snr).value_ref()));
      }
    }
  }
  printf("\n");
}

void printThresholds(const AlarmThreshold& thresholds) {
    printf("  Thresholds:\n");
    auto printMonitorThreshold = [](const char* monitor,
                                    const ThresholdLevels& levels) {
      printf(
          "  %-15s %-14s %.2f\n",
          monitor,
          "High Alarm",
          *(levels.alarm_ref()->high_ref()));
      printf(
          "  %-15s %-14s %.2f\n",
          monitor,
          "High Warning",
          *(levels.warn_ref()->high_ref()));
      printf(
          "  %-15s %-14s %.2f\n",
          monitor,
          "Low Warning",
          *(levels.warn_ref()->low_ref()));
      printf(
          "  %-15s %-14s %.2f\n",
          monitor,
          "Low Alarm",
          *(levels.alarm_ref()->low_ref()));
    };
    printMonitorThreshold("  Temp (C)", *(thresholds.temp_ref()));
    printMonitorThreshold("  Vcc (V)", *(thresholds.vcc_ref()));
    printMonitorThreshold("  Rx Power (mW)", *(thresholds.rxPwr_ref()));
    printMonitorThreshold("  Tx Power (mW)", *(thresholds.txPwr_ref()));
    printMonitorThreshold("  Tx Bias (mA)", *(thresholds.txBias_ref()));
}

void printManagementInterface(
    const TransceiverInfo& transceiverInfo,
    const char* fmt) {
  if (auto mgmtInterface =
          transceiverInfo.transceiverManagementInterface_ref()) {
    printf(fmt, apache::thrift::util::enumNameSafe(*mgmtInterface).c_str());
  }
}

void printVerboseInfo(const TransceiverInfo& transceiverInfo) {
  auto globalSensors = transceiverInfo.sensor_ref();

  if (globalSensors) {
    printGlobalInterruptFlags(*globalSensors);
  }
  printChannelInterruptFlags(*(transceiverInfo.channels_ref()));
  if (auto thresholds = transceiverInfo.thresholds_ref()) {
    printThresholds(*thresholds);
  }
}

void printVendorInfo(const TransceiverInfo& transceiverInfo) {
  if (auto vendor = transceiverInfo.vendor_ref()) {
    auto vendorInfo = *vendor;
    printf("  Vendor Info:\n");
    auto name = *(vendorInfo.name_ref());
    auto pn = *(vendorInfo.partNumber_ref());
    auto rev = *(vendorInfo.rev_ref());
    auto sn = *(vendorInfo.serialNumber_ref());
    auto dateCode = *(vendorInfo.dateCode_ref());
    printf("    Vendor: %s\n", name.c_str());
    printf("    Vendor PN: %s\n", pn.c_str());
    printf("    Vendor Rev: %s\n", rev.c_str());
    printf("    Vendor SN: %s\n", sn.c_str());
    printf("    Date Code: %s\n", dateCode.c_str());
  }
}

void printCableInfo(const Cable& cable) {
  printf("  Cable Settings:\n");
  if (auto sm = cable.singleMode_ref()) {
    printf("    Length (SMF): %d m\n", *sm);
  }
  if (auto om5 = cable.om5_ref()) {
    printf("    Length (OM5): %d m\n", *om5);
  }
  if (auto om4 = cable.om4_ref()) {
    printf("    Length (OM4): %d m\n", *om4);
  }
  if (auto om3 = cable.om3_ref()) {
    printf("    Length (OM3): %d m\n", *om3);
  }
  if (auto om2 = cable.om2_ref()) {
    printf("    Length (OM2): %d m\n", *om2);
  }
  if (auto om1 = cable.om1_ref()) {
    printf("    Length (OM1): %d m\n", *om1);
  }
  if (auto copper = cable.copper_ref()) {
    printf("    Length (Copper): %d m\n", *copper);
  }
  if (auto copperLength = cable.length_ref()) {
    printf("    Length (Copper dM): %.1f m\n", *copperLength);
  }
  if (auto gauge = cable.gauge_ref()) {
    printf("    DAC Cable Gauge: %d\n", *gauge);
  }
}

void printDomMonitors(const TransceiverInfo& transceiverInfo) {
  auto globalSensors = transceiverInfo.sensor_ref();
  printLaneDomMonitors(*(transceiverInfo.channels_ref()));
  if (globalSensors) {
    printGlobalDomMonitors(*globalSensors);
  }
}

void printSignalsAndSettings(const TransceiverInfo& transceiverInfo) {
  auto settings = *(transceiverInfo.settings_ref());
  if (auto hostSignals = transceiverInfo.hostLaneSignals_ref()) {
    printHostLaneSignals(*hostSignals);
  }
  if (auto mediaSignals = transceiverInfo.mediaLaneSignals_ref()) {
    printMediaLaneSignals(*mediaSignals);
  }
  if (auto hostSettings = settings.hostLaneSettings_ref()) {
    printHostLaneSettings(*hostSettings);
  }
  if (auto mediaSettings = settings.mediaLaneSettings_ref()) {
    printMediaLaneSettings(*mediaSettings);
  }
}

void printSffDetailService(
    const TransceiverInfo& transceiverInfo,
    unsigned int port,
    bool verbose) {
  auto settings = *(transceiverInfo.settings_ref());

  printf("Port %d\n", port);

  // ------ Module Status -------
  printf("  Module Status:\n");
  if (auto identifier = transceiverInfo.identifier_ref()) {
    printf(
        "    Transceiver Identifier: %s\n",
        apache::thrift::util::enumNameSafe(*identifier).c_str());
  }
  if (auto status = transceiverInfo.status_ref()) {
    printf("    InterruptL: 0x%02x\n", *(status->interruptL_ref()));
    printf("    Data_Not_Ready: 0x%02x\n", *(status->dataNotReady_ref()));
  }
  if (auto ext = transceiverInfo.extendedSpecificationComplianceCode_ref()) {
    printf(
        "    Extended Specification Compliance Code: %s\n",
        apache::thrift::util::enumNameSafe(*(ext)).c_str());
  }

  printManagementInterface(transceiverInfo, "    Transceiver Management Interface: %s\n");
  printSignalsAndSettings(transceiverInfo);
  printDomMonitors(transceiverInfo);

  if (verbose) {
    printVerboseInfo(transceiverInfo);
  }

  printVendorInfo(transceiverInfo);

  if (auto cable = transceiverInfo.cable_ref()) {
    printCableInfo(*cable);
  }
  printf("  Module Control:\n");
  printf(
      "    Rate Select: %s\n",
      apache::thrift::util::enumNameSafe(*(settings.rateSelect_ref())).c_str());
  printf(
      "    Rate Select Setting: %s\n",
      apache::thrift::util::enumNameSafe(*(settings.rateSelectSetting_ref()))
          .c_str());
  printf(
      "    CDR support:  TX: %s\tRX: %s\n",
      apache::thrift::util::enumNameSafe(*(settings.cdrTx_ref())).c_str(),
      apache::thrift::util::enumNameSafe(*(settings.cdrRx_ref())).c_str());
  printf(
      "    Power Measurement: %s\n",
      apache::thrift::util::enumNameSafe(*(settings.powerMeasurement_ref()))
          .c_str());
  printf(
      "    Power Control: %s\n",
      apache::thrift::util::enumNameSafe(*(settings.powerControl_ref()))
          .c_str());
  if (auto timeCollected = transceiverInfo.timeCollected_ref()) {
    printf("  Time collected: %s\n", getLocalTime(*timeCollected).c_str());
  }
}

void printSffDetail(const DOMDataUnion& domDataUnion, unsigned int port) {
  Sff8636Data sffData = domDataUnion.get_sff8636();
  auto lowerBuf = sffData.lower_ref()->data();
  auto page0Buf = sffData.page0_ref()->data();

  printf("Port %d\n", port);
  printf("  ID: %#04x\n", lowerBuf[0]);
  printf("  Status: 0x%02x 0x%02x\n", lowerBuf[1], lowerBuf[2]);
  printf("  Module State: 0x%02x\n", lowerBuf[3]);

  printf("  Interrupt Flags:\n");
  printf("    LOS: 0x%02x\n", lowerBuf[3]);
  printf("    Fault: 0x%02x\n", lowerBuf[4]);
  printf("    LOL: 0x%02x\n", lowerBuf[5]);
  printf("    Temp: 0x%02x\n", lowerBuf[6]);
  printf("    Vcc: 0x%02x\n", lowerBuf[7]);
  printf("    Rx Power: 0x%02x 0x%02x\n", lowerBuf[9], lowerBuf[10]);
  printf("    Tx Power: 0x%02x 0x%02x\n", lowerBuf[13], lowerBuf[14]);
  printf("    Tx Bias: 0x%02x 0x%02x\n", lowerBuf[11], lowerBuf[12]);
  printf("    Reserved Set 4: 0x%02x 0x%02x\n",
          lowerBuf[15], lowerBuf[16]);
  printf("    Reserved Set 5: 0x%02x 0x%02x\n",
          lowerBuf[17], lowerBuf[18]);
  printf("    Vendor Defined: 0x%02x 0x%02x 0x%02x\n",
         lowerBuf[19], lowerBuf[20], lowerBuf[21]);

  auto temp = static_cast<int8_t>
              (lowerBuf[22]) + (lowerBuf[23] / 256.0);
  printf("  Temperature: %f C\n", temp);
  uint16_t voltage = (lowerBuf[26] << 8) | lowerBuf[27];
  printf("  Supply Voltage: %f V\n", voltage / 10000.0);

  printf("  Channel Data:  %12s    %12s    %12s    %12s\n",
         "RX Power", "TX Power", "TX Bias", "Rx SNR");
  printChannelMonitor(1, lowerBuf, 34, 35, 42, 43, 50, 51);
  printChannelMonitor(2, lowerBuf, 36, 37, 44, 45, 52, 53);
  printChannelMonitor(3, lowerBuf, 38, 39, 46, 47, 54, 55);
  printChannelMonitor(4, lowerBuf, 40, 41, 48, 49, 56, 57);
  printf("    Power measurement is %s\n",
         (page0Buf[92] & 0x04) ? "supported" : "unsupported");
  printf("    Reported RX Power is %s\n",
         (page0Buf[92] & 0x08) ? "average power" : "OMA");

  printf("  Power set:  0x%02x\tExtended ID:  0x%02x\t"
         "Ethernet Compliance:  0x%02x\n",
         lowerBuf[93], page0Buf[1], page0Buf[3]);
  printf("  TX disable bits: 0x%02x\n", lowerBuf[86]);
  printf("  Rate select is %s\n",
      (page0Buf[93] & 0x0c) ? "supported" : "unsupported");
  printf("  RX rate select bits: 0x%02x\n", lowerBuf[87]);
  printf("  TX rate select bits: 0x%02x\n", lowerBuf[88]);
  printf("  CDR support:  TX: %s\tRX: %s\n",
      (page0Buf[1] & (1 << 3)) ? "supported" : "unsupported",
      (page0Buf[1] & (1 << 2)) ? "supported" : "unsupported");
  printf("  CDR bits: 0x%02x\n", lowerBuf[98]);

  auto vendor = sfpString(page0Buf, 20, 16);
  auto vendorPN = sfpString(page0Buf, 40, 16);
  auto vendorRev = sfpString(page0Buf, 56, 2);
  auto vendorSN = sfpString(page0Buf, 68, 16);
  auto vendorDate = sfpString(page0Buf, 84, 8);

  int gauge = page0Buf[109];
  auto cableGauge = gauge;
  if (gauge == eePromDefault && gauge > maxGauge) {
    // gauge implemented as hexadecimal (why?). Convert to decimal
    cableGauge = (gauge / hexBase) * decimalBase + gauge % hexBase;
  } else {
    cableGauge = 0;
  }

  printf("  Connector: 0x%02x\n", page0Buf[2]);
  printf("  Spec compliance: "
         "0x%02x 0x%02x 0x%02x 0x%02x"
         "0x%02x 0x%02x 0x%02x 0x%02x\n",
         page0Buf[3], page0Buf[4], page0Buf[5], page0Buf[6],
         page0Buf[7], page0Buf[8], page0Buf[9], page0Buf[10]);
  printf("  Encoding: 0x%02x\n", page0Buf[11]);
  printf("  Nominal Bit Rate: %d MBps\n", page0Buf[12] * 100);
  printf("  Ext rate select compliance: 0x%02x\n", page0Buf[13]);
  printf("  Length (SMF): %d km\n", page0Buf[14]);
  printf("  Length (OM3): %d m\n", page0Buf[15] * 2);
  printf("  Length (OM2): %d m\n", page0Buf[16]);
  printf("  Length (OM1): %d m\n", page0Buf[17]);
  printf("  Length (Copper): %d m\n", page0Buf[18]);
  if (page0Buf[108] != eePromDefault) {
    auto fractional = page0Buf[108] * .1;
    auto effective = fractional >= 1 ? fractional : page0Buf[18];
    printf("  Length (Copper dM): %.1f m\n", fractional);
    printf("  Length (Copper effective): %.1f m\n", effective);
  }
  if (cableGauge > 0){
    printf("  DAC Cable Gauge: %d\n", cableGauge);
  }
  printf("  Device Tech: 0x%02x\n", page0Buf[19]);
  printf("  Ext Module: 0x%02x\n", page0Buf[36]);
  printf("  Wavelength tolerance: 0x%02x 0x%02x\n",
         page0Buf[60], page0Buf[61]);
  printf("  Max case temp: %dC\n", page0Buf[62]);
  printf("  CC_BASE: 0x%02x\n", page0Buf[63]);
  printf("  Options: 0x%02x 0x%02x 0x%02x 0x%02x\n",
         page0Buf[64], page0Buf[65],
         page0Buf[66], page0Buf[67]);
  printf("  DOM Type: 0x%02x\n", page0Buf[92]);
  printf("  Enhanced Options: 0x%02x\n", page0Buf[93]);
  printf("  Reserved: 0x%02x\n", page0Buf[94]);
  printf("  CC_EXT: 0x%02x\n", page0Buf[95]);
  printf("  Vendor Specific:\n");
  printf("    %02x %02x %02x %02x %02x %02x %02x %02x"
         "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
         page0Buf[96], page0Buf[97], page0Buf[98], page0Buf[99],
         page0Buf[100], page0Buf[101], page0Buf[102], page0Buf[103],
         page0Buf[104], page0Buf[105], page0Buf[106], page0Buf[107],
         page0Buf[108], page0Buf[109], page0Buf[110], page0Buf[111]);
  printf("    %02x %02x %02x %02x %02x %02x %02x %02x"
         "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
         page0Buf[112], page0Buf[113], page0Buf[114], page0Buf[115],
         page0Buf[116], page0Buf[117], page0Buf[118], page0Buf[119],
         page0Buf[120], page0Buf[121], page0Buf[122], page0Buf[123],
         page0Buf[124], page0Buf[125], page0Buf[126], page0Buf[127]);

  printf("  Vendor: %s\n", vendor.str().c_str());
  printf("  Vendor OUI: %02x:%02x:%02x\n", lowerBuf[165 - 128],
          lowerBuf[166 - 128], lowerBuf[167 - 128]);
  printf("  Vendor PN: %s\n", vendorPN.str().c_str());
  printf("  Vendor Rev: %s\n", vendorRev.str().c_str());
  printf("  Vendor SN: %s\n", vendorSN.str().c_str());
  printf("  Date Code: %s\n", vendorDate.str().c_str());

  // print page3 values
  if (!sffData.page3_ref()) {
    return;
  }

  auto page3Buf = sffData.page3_ref().value_unchecked().data();

  printThresholds("Temp", &page3Buf[0], [](const uint16_t u16_temp) {
    double data;
    data = u16_temp / 256.0;
    if (data > 128) {
      data = data - 256;
    }
    return data;
  });

  printThresholds("Vcc", &page3Buf[16], [](const uint16_t u16_vcc) {
    double data;
    data = u16_vcc / 10000.0;
    return data;
  });

  printThresholds("Rx Power", &page3Buf[48], [](const uint16_t u16_rxpwr) {
    double data;
    data = u16_rxpwr * 0.1 / 1000;
    return data;
  });

  printThresholds("Tx Bias", &page3Buf[56], [](const uint16_t u16_txbias) {
    double data;
    data = u16_txbias * 2.0 / 1000;
    return data;
  });

  if (auto timeCollected = sffData.timeCollected_ref()) {
    printf(
      "  Time collected: %s\n",
      getLocalTime(*timeCollected).c_str());
  }
}

void printCmisDetailService(
    const TransceiverInfo& transceiverInfo,
    unsigned int port,
    bool verbose) {
  auto moduleStatus = transceiverInfo.status_ref();
  auto channels = *(transceiverInfo.channels_ref());
  printf("Port %d\n", port);
  auto settings = *(transceiverInfo.settings_ref());

  printManagementInterface(transceiverInfo, "  Transceiver Management Interface: %s\n");

  if (moduleStatus && moduleStatus->cmisModuleState_ref()) {
    printf(
        "  Module State: %s\n",
        apache::thrift::util::enumNameSafe(
            *(moduleStatus->cmisModuleState_ref()))
            .c_str());
  }
  if (auto mediaInterfaceId = settings.mediaInterface_ref()) {
    printf(
        "  Media Interface: %s\n",
        apache::thrift::util::enumNameSafe(
            (*mediaInterfaceId)[0].media_ref()->get_smfCode())
            .c_str());
  }
  printf(
      "  Power Control: %s\n",
      apache::thrift::util::enumNameSafe(*settings.powerControl_ref()).c_str());

  if (moduleStatus && moduleStatus->fwStatus_ref()) {
    auto fwStatus = *(moduleStatus->fwStatus_ref());
    if (auto version = fwStatus.version_ref()) {
      printf("  FW Version: %s\n", (*version).c_str());
    }
    if (auto fwFault = fwStatus.fwFault_ref()) {
      printf("  Firmware fault: 0x%x\n", *fwFault);
    }
  }

  if (verbose) {
    printVerboseInfo(transceiverInfo);
  }
  printSignalsAndSettings(transceiverInfo);
  printDomMonitors(transceiverInfo);
  printVendorInfo(transceiverInfo);
  if (auto timeCollected = transceiverInfo.timeCollected_ref()) {
    printf("  Time collected: %s\n", getLocalTime(*timeCollected).c_str());
  }
}

void printCmisDetail(const DOMDataUnion& domDataUnion, unsigned int port) {
  int i = 0; // For the index of lane
  CmisData cmisData = domDataUnion.get_cmis();
  auto lowerBuf = cmisData.lower_ref()->data();
  auto page0Buf = cmisData.page0_ref()->data();
  auto page01Buf = can_throw(cmisData.page01_ref())->data();
  auto page10Buf = can_throw(cmisData.page10_ref())->data();
  auto page11Buf = can_throw(cmisData.page11_ref())->data();
  auto page14Buf = can_throw(cmisData.page14_ref())->data();

  printf("Port %d\n", port);
  printf("  Module Interface Type: CMIS (200G or above)\n");

  printf("  Module State: %s\n",
      getStateNameString(lowerBuf[3] >> 1, kCmisModuleStateMapping).c_str());

  auto ApSel = page11Buf[78] >> 4;
  auto ApCode = lowerBuf[86 + (ApSel - 1) * 4 + 1];
  printf("  Application Selected: %s\n",
      getStateNameString(ApCode, kCmisAppNameMapping).c_str());
  printf("  Low power: 0x%x\n", (lowerBuf[26] >> 6) & 0x1);
  printf("  Low power forced: 0x%x\n", (lowerBuf[26] >> 4) & 0x1);

  printf("  Module FW Version: %x.%x\n", lowerBuf[39], lowerBuf[40]);
  printf("  DSP FW Version: %x.%x\n", page01Buf[66], page01Buf[67]);
  printf("  Build Rev: %x.%x\n", page01Buf[68], page01Buf[69]);
  printf("  Firmware fault: 0x%x\n", (lowerBuf[8] >> 1) & 0x3);
  auto vendor = sfpString(page0Buf, 1, 16);
  auto vendorPN = sfpString(page0Buf, 20, 16);
  auto vendorRev = sfpString(page0Buf, 36, 2);
  auto vendorSN = sfpString(page0Buf, 38, 16);
  auto vendorDate = sfpString(page0Buf, 54, 8);

  printf("  Vendor: %s\n", vendor.str().c_str());
  printf("  Vendor PN: %s\n", vendorPN.str().c_str());
  printf("  Vendor Rev: %s\n", vendorRev.str().c_str());
  printf("  Vendor SN: %s\n", vendorSN.str().c_str());
  printf("  Date Code: %s\n", vendorDate.str().c_str());

  auto temp = static_cast<int8_t>
              (lowerBuf[14]) + (lowerBuf[15] / 256.0);
  printf("  Temperature: %f C\n", temp);

  printf("  VCC: %f V\n", CmisFieldInfo::getVcc(lowerBuf[16] << 8 | lowerBuf[17]));

  printf("\nPer Lane status: \n");
  printf("Lanes             1        2        3        4        5        6        7        8\n");
  printf("Datapath de-init  ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page10Buf[0]>>i)&1);
  }
  printf("\nTx disable        ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page10Buf[2]>>i)&1);
  }
  printf("\nTx squelch bmap   ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page10Buf[4]>>i)&1);
  }
  printf("\nRx Out disable    ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page10Buf[10]>>i)&1);
  }
  printf("\nRx Sqlch disable  ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page10Buf[11]>>i)&1);
  }
  printf("\nHost lane state   ");
  for (i=0; i<4; i++) {
    printf("%-7s  %-7s  ",
    getStateNameString(page11Buf[i] & 0xf, kCmisLaneStateMapping).c_str(),
    getStateNameString((page11Buf[i]>>4) & 0xf, kCmisLaneStateMapping).c_str());
  }
  printf("\nTx fault          ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[7]>>i)&1);
  }
  printf("\nTx LOS            ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[8]>>i)&1);
  }
  printf("\nTx LOL            ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[9]>>i)&1);
  }
  printf("\nTx PWR alarm Hi   ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[11]>>i)&1);
  }
  printf("\nTx PWR alarm Lo   ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[12]>>i)&1);
  }
  printf("\nTx PWR warn Hi    ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[13]>>i)&1);
  }
  printf("\nTx PWR warn Lo    ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[14]>>i)&1);
  }
  printf("\nRx LOS            ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[19]>>i)&1);
  }
  printf("\nRx LOL            ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[20]>>i)&1);
  }
  printf("\nRx PWR alarm Hi   ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[21]>>i)&1);
  }
  printf("\nRx PWR alarm Lo   ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[22]>>i)&1);
  }
  printf("\nRx PWR warn Hi    ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[23]>>i)&1);
  }
  printf("\nRx PWR warn Lo    ");
  for (i = 0; i < 8; i++) {
    printf("%d        ", (page11Buf[24]>>i)&1);
  }
  printf("\nTX Power (mW)     ");
  for (i = 0; i < 8; i++) {
    printf("%.3f    ", ((page11Buf[26+i*2]<<8 | page11Buf[27+i*2]))*0.0001);
  }
  printf("\nRX Power (mW)     ");
  for (i = 0; i < 8; i++) {
    printf("%.3f    ", ((page11Buf[58+i*2]<<8 | page11Buf[59+i*2]))*0.0001);
  }
  printf("\nRx SNR            ");
  for (i = 0; i < 8; i++) {
    printf("%05.04g    ", (CmisFieldInfo::getSnr(page14Buf[113+i*2] << 8 | page14Buf[112+i*2])));
  }
  printf("\nRX PreCur (dB)    ");
  for (i = 0; i < 8; i++) {
    printf("%.1f      ", CmisFieldInfo::getPreCursor((page11Buf[95+i/2]>>((i%2)*4)) & 0x0f));
  }
  printf("\nRX PostCur (dB)   ");
  for (i = 0; i < 8; i++) {
    printf("%.1f      ", CmisFieldInfo::getPostCursor((page11Buf[99+i/2]>>((i%2)*4)) & 0x0f));
  }
  printf("\nRX Ampl (mV)      ");
  for (i = 0; i < 8; i++) {
    auto ampRange = CmisFieldInfo::getAmplitude((page11Buf[103+i/2]>>((i%2)*4)) & 0x0f);
    char rangeStr[16];
    sprintf(rangeStr, "%d-%d      ", ampRange.first, ampRange.second);
    rangeStr[9] = 0;
    printf("%s", rangeStr);
  }
  if (auto timeCollected = cmisData.timeCollected_ref()) {
    printf(
      "\nTime collected: %s",
      getLocalTime(*timeCollected).c_str());
  }
  printf("\n\n");
}

void printPortDetail(const DOMDataUnion& domDataUnion, unsigned int port) {
  if (domDataUnion.__EMPTY__) {
    fprintf(stderr, "DOMDataUnion object is empty\n");
    return;
  }
  if (domDataUnion.getType() == DOMDataUnion::Type::sff8636) {
    printSffDetail(domDataUnion, port);
  } else if (domDataUnion.getType() == DOMDataUnion::Type::cmis) {
    printCmisDetail(domDataUnion, port);
  } else {
    fprintf(stderr, "Unrecognizable DOMDataUnion format.\n");
    return;
  }
}

void printPortDetailService(
    const TransceiverInfo& transceiverInfo,
    unsigned int port,
    bool verbose) {
  if (auto mgmtInterface =
          transceiverInfo.transceiverManagementInterface_ref()) {
    if (*mgmtInterface == TransceiverManagementInterface::SFF) {
      printSffDetailService(transceiverInfo, port, verbose);
    } else {
      assert(*mgmtInterface == TransceiverManagementInterface::CMIS);
      printCmisDetailService(transceiverInfo, port, verbose);
    }
  } else {
    fprintf(
        stderr,
        "Management Interface cannot be inferred. Port %d may not be present.\n",
        port);
  }
}

bool isTrident2() {
  std::string contents;
  if (!folly::readFile(chipCheckPath, contents)) {
    if (errno == ENOENT) {
      return false;
    }
    folly::throwSystemError("error reading ", chipCheckPath);
  }
  return (contents == trident2);
}

void tryOpenBus(TransceiverI2CApi* bus) {
  auto expireTime = steady_clock::now() + seconds(FLAGS_open_timeout);
  while (true) {
    try {
      bus->open();
      return;
    } catch (const std::exception& ex) {
      if (steady_clock::now() > expireTime) {
        throw;
      }
    }
    usleep(100);
  }
}

/* This function does a hard reset of the QSFP in a given platform. The reset
 * is done by Fpga or I2C function. This function calls another function which
 * creates and returns TransceiverPlatformApi object. For Fpga controlled
 * platform the called function creates Platform specific TransceiverApi object
 * and returns it. For I2c controlled platform the called function creates
 * TransceiverPlatformI2cApi object and keeps the platform specific I2CBus
 * object raw pointer inside it. The returned object's Qsfp control function
 * is called here to use appropriate Fpga/I2c Api in this function.
 */
bool doQsfpHardReset(
  TransceiverI2CApi *bus,
  unsigned int port) {

  // Call the function to get TransceiverPlatformApi object. For Fpga
  // controlled platform it returns Platform specific TransceiverApi object.
  // For I2c controlled platform it returns TransceiverPlatformI2cApi
  // which contains "bus" as it's internal variable
  auto busAndError = getTransceiverPlatformAPI(bus);

  if (busAndError.second) {
    fprintf(stderr, "Trying to doQsfpHardReset, Couldn't getTransceiverPlatformAPI, error out.\n");
      return false;
  }

  auto qsfpBus = std::move(busAndError.first);

  // This will eventuall call the Fpga or the I2C based platform specific
  // Qsfp reset function
  qsfpBus->triggerQsfpHardReset(port);

  return true;
}

bool doMiniphotonLoopback(TransceiverI2CApi* bus, unsigned int port, LoopbackMode mode) {
  try {
    // Make sure we have page128 selected.
    uint8_t page128 = 128;
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 127, 1, &page128);

    uint8_t loopback = 0;
    if (mode == electricalLoopback) {
      loopback = 0b01010101;
    } else if (mode == opticalLoopback) {
      loopback = 0b10101010;
    }
    fprintf(stderr, "loopback value: %x\n", loopback);
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 245, 1, &loopback);
  } catch (const I2cError& ex) {
    fprintf(stderr, "QSFP %d: fail to set loopback\n", port);
    return false;
  }
  return true;
}

void cmisHostInputLoopback(TransceiverI2CApi* bus, unsigned int port, LoopbackMode mode) {
  try {
    // Make sure we have page 0x13 selected.
    uint8_t page = 0x13;
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 127, sizeof(page), &page);

    uint8_t data = (mode == electricalLoopback) ? 0xff : 0;
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 183, sizeof(data), &data);
  } catch (const I2cError& ex) {
    fprintf(stderr, "QSFP %d: fail to set loopback\n", port);
  }
}

void cmisMediaInputLoopback(TransceiverI2CApi* bus, unsigned int port, LoopbackMode mode) {
  try {
    // Make sure we have page 0x13 selected.
    uint8_t page = 0x13;
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 127, sizeof(page), &page);

    uint8_t data;
    bus->moduleRead(port, TransceiverI2CApi::ADDR_QSFP, 128, sizeof(data), &data);
    if (!(data & 0x02)) {
      fprintf(stderr, "QSFP %d: Media side input loopback not supported\n", port);
      return;
    }

    data = (mode == opticalLoopback) ? 0xff : 0;
    bus->moduleWrite(port, TransceiverI2CApi::ADDR_QSFP, 181, sizeof(data), &data);
  } catch (const I2cError& ex) {
    fprintf(stderr, "QSFP %d: fail to set loopback\n", port);
  }
}

/*
 * getPidForProcess
 *
 * This function returns process id (or a list of process ids) for a given
 * process name. This function looks into the proc entries and search for
 * the process name in all these files: /proc/<pid>/cmdline
 * It will return list of PID corresponding to the process name. If no such
 * process found then the return list will be empty.
 */
std::vector<int> getPidForProcess(std::string proccessName)
{
  std::vector<int> pidList;

  // Look into the /proc directory
  DIR *dp = opendir("/proc");
  if (dp == nullptr) {
    return pidList;
  }

  // Look for all /proc/<pid>/cmdline to check if anything matches with
  // input processName
  struct dirent *dirEntry;
  while ((dirEntry = readdir(dp))) {
    int currPid = atoi(dirEntry->d_name);
    if (currPid <= 0) {
      // skip non user processes
      continue;
    }

    // This is proper user process, read cmdline
    std::string commandPath = std::string("/proc/") + dirEntry->d_name + "/cmdline";
    std::ifstream commandFile(commandPath.c_str());
    std::string commandLine;
    getline(commandFile, commandLine);
    if (commandLine.empty()) {
      continue;
    }

    // The commandline could be like /tmp/qsfp_service --macsec_bypass
    // Keep only first word and remove directory names
    size_t position = commandLine.find('\0');
    if (position != std::string::npos) {
      commandLine = commandLine.substr(0, position);
    }
    position = commandLine.find('/');
    if (position != std::string::npos) {
      commandLine = commandLine.substr(position + 1);
    }

    // Now compare the process name
    if (commandLine == proccessName) {
      pidList.push_back(currPid);
    }
  }
  closedir(dp);
  return pidList;
}

/*
 * cliModulefirmwareUpgrade
 *
 * This function does the firmware upgrade on the optics module in the current
 * thread. This directly invokes the bus->moduleRead/Write API to do the module
 * CDB based firnware upgrade
 */
bool cliModulefirmwareUpgrade(
  TransceiverI2CApi* bus, unsigned int port, std::string firmwareFilename) {

  // If the qsfp_service is running then this firmware upgrade command is most
  // likely to fail. Print warning and returning from here
  auto pidList = getPidForProcess("qsfp_service");
  if (!pidList.empty()) {
    printf("The qsfp_service seems to be running.\n");
    printf("The f/w upgrade CLI may not work reliably\n");
    printf("Consider stopping qsfp_service and re-issue this upgrade command\n");
    return false;
  }

  auto domData = fetchDataFromLocalI2CBus (bus, port);
  CmisData cmisData = domData.get_cmis();
  auto dataUpper = cmisData.page0_ref()->data();

  // Confirm module type is CMIS
  auto moduleType = getModuleType(bus, port);
  if (moduleType != TransceiverManagementInterface::CMIS) {
    // If device can't be determined as cmis then check page 0 register 128 also to identify its
    // type as in some case corrupted image wipes out all lower page registers
    if (dataUpper[0] == static_cast<uint8_t>(TransceiverModuleIdentifier::QSFP_PLUS_CMIS) ||
        dataUpper[0] == static_cast<uint8_t>(TransceiverModuleIdentifier::QSFP_DD)) {
          printf("Optics current firmware seems to  be corrupted, proceeding with f/w upgrade\n");
    } else {
      printf("This command is applicable to CMIS module only\n");
      printf("This module does not appears to be CMIS module, exiting...\n");
    }
  }

  // Get the image header length
  uint32_t imageHdrLen = 0;
  if (FLAGS_image_header_len > 0) {
    imageHdrLen = FLAGS_image_header_len;
  } else {
    // Image header length is not provided by user. Try to get it from known
    // module info
    std::array<uint8_t, 16> modPartNo;
    for (int i=0; i<16; i++) {
      modPartNo[i] = dataUpper[20+i];
    }

    for (int i=0; i<kNumModuleInfo; i++) {
      if (modulePartInfo[i].partNo == modPartNo) {
        imageHdrLen = modulePartInfo[i].headerLen;
        break;
      }
    }
    if (imageHdrLen == 0) {
      printf("Image header length is not specified on command line and");
      printf(" the default image header size is unknown for this module");
      printf("Pl re-run the same command with option --image_header_len <len>");
      return false;
    }
  }

  // Create FbossFirmware object using firmware filename and msa password,
  // header length as properties
  FbossFirmware::FwAttributes firmwareAttr;
  firmwareAttr.filename = firmwareFilename;
  firmwareAttr.properties["msa_password"] = folly::to<std::string>(FLAGS_msa_password);
  firmwareAttr.properties["header_length"] = folly::to<std::string>(imageHdrLen);
  firmwareAttr.properties["image_type"] = FLAGS_dsp_image ? "dsp" : "application";
  auto fbossFwObj = std::make_unique<FbossFirmware>(firmwareAttr);

  auto fwUpgradeObj = std::make_unique<CmisFirmwareUpgrader>(
    bus, port, std::move(fbossFwObj));

  // Do the standalone upgrade in the same process as wedge_qsfp_util
  bool ret = fwUpgradeObj->cmisModuleFirmwareUpgrade();

  if (ret) {
    printf("Firmware download successful, the module is running desired firmware\n");
    printf("Pl reload the chassis to finish the last step\n");
  } else {
    printf("Firmware upgrade failed, you may retry the same command\n");
  }

  return ret;
}

/*
 * cliModulefirmwareUpgrade
 *
 * This is multi-threaded version of the module upgrade trigger function.
 * This multi-threaded overloaded function gets the list of optics on which
 * the upgrade needs to be performed. It calls the function bucketize to create
 * separate buckets of optics for upgrade. Then the threads are created for
 * each bucket and each thread takes care of upgrading the optics in that
 * bucket. The idea here is that one thread will take care of upgrading the
 * optics belonging to one controller so that two ports of the same controller
 * can't upgrade at the same time whereas multiple ports belonging to different
 * controller can upgrade at the same time. This function waits for all
 * threads to finish.
 */
bool cliModulefirmwareUpgrade(
  TransceiverI2CApi* bus,
  std::string portRangeStr,
  std::string firmwareFilename) {

  std::vector<std::thread> threadList;
  std::vector<std::vector<unsigned int>> bucket;

  // Check if the filename is specified
  if (firmwareFilename.empty()) {
    fprintf(stderr, "Firmware filename not specified for upgrade\n");
    return false;
  }

  // Convert the port regex string to list of ports
  std::vector<unsigned int> modlist = portRangeStrToPortList(portRangeStr);

  // Get the list of all the modules where upgrade can be done
  std::vector<unsigned int> finalModlist = getUpgradeModList(
    bus, modlist, FLAGS_module_type, FLAGS_fw_version);

  // Get the modules per controller (platform specific)
  int modsPerController = getModulesPerController();

  // Bucketize the list of modules among buckets as per the controllers
  bucketize(finalModlist, modsPerController, bucket);

  printf("The modules will be upgraded in these %ld buckets\n", bucket.size());
  printf("Each bucket will be assigned a separate thread\n");

  for (auto& bucketrow : bucket) {
    printf("Bucket: ");
    for (auto& module : bucketrow) {
      printf("%d ", module);
    }
    printf("\n");
  }

  /* sleep override */
  sleep(10);

  // Get the image header length
  uint32_t imageHdrLen = 0;
  if (FLAGS_image_header_len > 0) {
    imageHdrLen = FLAGS_image_header_len;
  } else {
    // Image header length is not provided by user. Try to get it from known
    // module info frm first module in the list
    auto domData = fetchDataFromLocalI2CBus (bus, bucket[0][0]);
    CmisData cmisData = domData.get_cmis();
    auto dataUpper = cmisData.page0_ref()->data();

    std::array<uint8_t, 16> modPartNo;
    for (int i=0; i<16; i++) {
      modPartNo[i] = dataUpper[20+i];
    }

    for (int i=0; i<kNumModuleInfo; i++) {
      if (modulePartInfo[i].partNo == modPartNo) {
        imageHdrLen = modulePartInfo[i].headerLen;
        break;
      }
    }
    if (imageHdrLen == 0) {
      printf("Image header length is not specified on command line and");
      printf(" the default image header size is unknown for this module");
      printf("Pl re-run the same command with option --image_header_len <len>");
      return false;
    }
  }

  // Create threads one for each bucket row and do the upgrade there
  for (auto& bucketrow : bucket) {
    std::thread tHandler(
        fwUpgradeThreadHandler, bus, bucketrow, firmwareFilename, imageHdrLen);
    threadList.push_back(std::move(tHandler));
  }

  // Wait for all threads to finish
  for (auto& tHandler : threadList) {
    tHandler.join();
  }

  printf("Firmware upgrade done on some of the modules");
  printf(
      "Check the status using: wedge_qsfp_util --get_module_fw_info <portA> <portB>\n");
  printf("Pl reload the chassis to finish the firmware upgrade last step\n");
  return true;
}

/*
 * fwUpgradeThreadHandler
 *
 * This is Entry function for the thread which will do the firmware upgrade
 * operation. This function gets the list of optics as an argument. It will do
 * fw upgrade on all these modules one by one in this thread context.
 */
void fwUpgradeThreadHandler(
    TransceiverI2CApi* bus,
    std::vector<unsigned int> modlist,
    std::string firmwareFilename,
    uint32_t imageHdrLen) {
  std::array<uint8_t, 2> versionNumber;

  for (auto module : modlist) {
    // Create FbossFirmware object using firmware filename and msa password,
    // header length as properties
    FbossFirmware::FwAttributes firmwareAttr;
    firmwareAttr.filename = firmwareFilename;
    firmwareAttr.properties["msa_password"] = folly::to<std::string>(FLAGS_msa_password);
    firmwareAttr.properties["header_length"] = folly::to<std::string>(imageHdrLen);
    firmwareAttr.properties["image_type"] = FLAGS_dsp_image ? "dsp" : "application";
    auto fbossFwObj = std::make_unique<FbossFirmware>(firmwareAttr);

    auto fwUpgradeObj = std::make_unique<CmisFirmwareUpgrader>(
      bus, module, std::move(fbossFwObj));

    // Do the upgrade in this thread
    bool ret = fwUpgradeObj->cmisModuleFirmwareUpgrade();

    if (ret) {
      printf("Firmware download successful for module %d, the module is running desired firmware\n", module);
    } else {
      printf("Firmware upgrade failed for module %d, you may retry the same command\n", module);
    }

    // Find out the current version running on module
    bus->moduleRead(module, TransceiverI2CApi::ADDR_QSFP, 39, 2, versionNumber.data());
    printf(
        "cmisModuleFirmwareUpgrade: Mod%d: Module Active Firmware Revision now: %d.%d\n",
        module,
        versionNumber[0],
        versionNumber[1]);
  }
}

/*
 * getUpgradeModList
 *
 * This function goes through all the modules in the range specified by portA
 * to portB (inclusive) and finds the modules which are eligible for firmware
 * upgrade. The eligible modules meet these criteria:
 *   - Module is present
 *   - Module is CMIS type
 *   - Module is Module type specified matches with mapped module part number
 *     from its register page 1 byte 148-163
 *   - Module's current running version is not same as specified in input
 *     argument
 *
 * This function returns the list of such eligible modules for firmware upgrade
 */
std::vector<unsigned int> getUpgradeModList(
    TransceiverI2CApi* bus,
    std::vector<unsigned int> portlist,
    std::string moduleType,
    std::string fwVer) {
  std::string partNoStr;
  std::vector<unsigned int> modlist;

  // Check if we have the mapping for this user specified module type to part
  // number
  if (CmisFirmwareUpgrader::partNoMap.find(moduleType) == CmisFirmwareUpgrader::partNoMap.end()) {
    printf("Module Type is invalid\n");
    return modlist;
  }

  partNoStr = CmisFirmwareUpgrader::partNoMap[moduleType];

  for (unsigned int module : portlist) {
    std::array<uint8_t, 16> partNo;
    std::array<uint8_t, 2> fwVerNo;
    std::array<char, 16> baseFwVerStr;
    std::string tempPartNo, tempFwVerStr;

    // Check if the module is present
    if (!bus->isPresent(module)) {
      continue;
    }

    auto qsfpImpl = std::make_unique<WedgeQsfp>(module - 1, bus);
    auto mgmtInterface = qsfpImpl->getTransceiverManagementInterface();

    // Check if it is CMIS module
    if (mgmtInterface != TransceiverManagementInterface::CMIS) {
      continue;
    }

    fwVerNo = qsfpImpl->getFirmwareVer();
    partNo = qsfpImpl->getModulePartNo();

    // Check if module is of same type specified in argument
    for (int i = 0; i < 16; i++) {
      tempPartNo += partNo[i];
    }
    if (tempPartNo != partNoStr) {
      continue;
    }

    // Check if the moduleis not already running the same f/w version
    sprintf(baseFwVerStr.data(), "%x.%x", fwVerNo[0], fwVerNo[1]);
    tempFwVerStr = baseFwVerStr.data();
    if (tempFwVerStr == fwVer) {
      continue;
    }

    modlist.push_back(module);
  }
  return modlist;
}

/*
 * bucketize
 *
 * This function takes the list of optics to upgrade the firmware and then
 * bucketize them to the rows where each row contains the list of optics
 * which can be upgraded in a thread. For example: In yamp platform which has
 * one i2c controller for 8 modules, if the input  is the list of these modules:
 *  1 3 5 8 12 15 21 32 38 46 55 59 60 83 88 122 124 128
 * Then these will be divided in following 9 buckets:
 *  Bucket: 1 3 5 8
 *  Bucket: 12 15
 *  Bucket: 21
 *  Bucket: 32 38
 *  Bucket: 46
 *  Bucket: 55
 *  Bucket: 59 60
 *  Bucket: 83 88
 *  Bucket: 122 124 128
 */
void bucketize(
    std::vector<unsigned int>& modlist,
    int modulesPerController,
    std::vector<std::vector<unsigned int>>& bucket) {
  int currBucketId, currBucketEnd;
  std::vector<unsigned int> tempModList;

  std::sort(modlist.begin(), modlist.end());

  currBucketId = -1;

  for (auto module : modlist) {
    currBucketEnd = (currBucketId + 1) * modulesPerController;

    if (module > currBucketEnd) {
      if (!tempModList.empty()) {
        bucket.push_back(tempModList);
        tempModList.clear();
      }
      currBucketId = module / modulesPerController;
      if (module % modulesPerController == 0) {
        currBucketId--;
      }
    }

    tempModList.push_back(module);
  }
  if (!tempModList.empty()) {
    bucket.push_back(tempModList);
    tempModList.clear();
  }
}

/*
 * portRangeStrToPortList
 *
 * This function takes port range string which is a string containing the
 * range of applicable ports and creates a list of port. The string could
 * be list of port and ranges, something like "1,2,3-9,12-17,29". This
 * function will expand, create and return a vector of integers:
 * {1,2,3,4,5,6,7,8,9,12,13,14,15,16,17,29}
 */
std::vector<unsigned int> portRangeStrToPortList(
    std::string portQualifier) {
  int lastPos = 0, nextPos;
  int beg, end;
  bool emptyStr;
  std::string tempString;
  std::vector<std::string> portRangeStrings;
  std::vector<unsigned int> portRangeList;

  // Create a list of substrings separated by command ","
  // These substrings will contain single port like "5" or port range like "7-9"
  while ((nextPos = portQualifier.find(std::string(","), lastPos)) !=
         std::string::npos) {
    tempString = portQualifier.substr(lastPos, nextPos - lastPos);
    lastPos = nextPos + 1;
    portRangeStrings.push_back(tempString);
  }
  tempString = portQualifier.substr(lastPos);
  portRangeStrings.push_back(tempString);

  // Convert this list of substrings to the list of pairs showing range.
  // The substrings "5","7-9","15-31" will be converted to list of pairs like:
  // {{5,5},{7,9},{15,31}}
  for (auto rangeOfPort : portRangeStrings) {
    lastPos = 0;
    nextPos = rangeOfPort.find(std::string("-"), lastPos);
    if (nextPos == std::string::npos) {
      emptyStr = std::all_of(rangeOfPort.begin(), rangeOfPort.end(), isspace);
      if (emptyStr) {
        continue;
      }
      beg = end = std::stoi(rangeOfPort);
    } else {
      tempString = rangeOfPort.substr(lastPos, nextPos - lastPos);
      beg = std::stoi(tempString);
      lastPos = nextPos + 1;
      tempString = rangeOfPort.substr(lastPos);
      end = std::stoi(tempString);
    }
    for (int i = beg; i <= end; i++) {
      portRangeList.push_back(i);
    }
  }
  // Sort the list
  std::sort(portRangeList.begin(), portRangeList.end());

  return portRangeList;
}

/*
 * get_module_fw_info
 *
 * This function gets the module firmware info and prints it for a range of
 * ports. The info are : vendor name, part number and current firmware version
 * sample output:
 * Module     Vendor               Part Number          Fw version
 * 52         FINISAR CORP.        FTCC1112E1PLL-FB     2.1
 * 82         INNOLIGHT            T-FX4FNT-HFB         ca.f8
 * 84         FINISAR CORP.        FTCC1112E1PLL-FB     7.8
 */
void get_module_fw_info(TransceiverI2CApi* bus, unsigned int moduleA, unsigned int moduleB) {

  if (moduleA > moduleB) {
    printf("The moduleA should be smaller than or equal to moduleB\n");
    return;
  }

  printf("Displaying firmware info for modules %d-%d\n", moduleA, moduleB);
  printf("Module     Vendor               Part Number          Fw version\n");

  for (unsigned int module = moduleA; module <= moduleB; module++) {

    std::array<uint8_t, 16> vendor;
    std::array<uint8_t, 16> partNo;
    std::array<uint8_t, 2> fwVer;

    if(!bus->isPresent(module)) {
        continue;
      }

    auto moduleType = getModuleType(bus, module);
    if (moduleType != TransceiverManagementInterface::CMIS) {
      continue;
    }

    DOMDataUnion tempDomData = fetchDataFromLocalI2CBus(bus, module);
    CmisData cmisData = tempDomData.get_cmis();
    auto dataLower = cmisData.lower_ref()->data();
    auto dataUpper = cmisData.page0_ref()->data();

    fwVer[0] = dataLower[39];
    fwVer[1] = dataLower[40];

    memcpy(&vendor[0], &dataUpper[1], 16);
    memcpy(&partNo[0], &dataUpper[20], 16);

    printf("%2d         ", module);
    for (int i=0; i<16; i++) {
      printf("%c", vendor[i]);
    }

    printf("     ");
    for (int i=0; i<16; i++) {
      printf("%c", partNo[i]);
    }

    printf("     ");
    printf("%x.%x", fwVer[0], fwVer[1]);
    printf("\n");
  }
}

/*
 * doCdbCommand()
 * Read from the module CDB block. The syntax are:
 *
 * [1] wedge_qsfp_util <module-id> --cdb_command --command_code <command-code>
 *     Here the command code is 16 bit command
 *
 * [2] wedge_qsfp_util <module-id> --cdb_command --command_code <command-code> --cdb_payload <cdb-payload>
 *     Here the cdb-payload is a string containing payload bytes, ie: "100 200"
 */

void doCdbCommand(TransceiverI2CApi* bus,  unsigned int  module) {
  if (FLAGS_command_code == -1) {
    printf("A valid command code is requied\n");
    return;
  }

  uint16_t commandCode = FLAGS_command_code & 0xffff;

  std::vector<uint8_t> lplMem;

  if (!FLAGS_cdb_payload.empty()) {
    // Convert the string of integers to stringstream and then push back all
    // uint8_t from it to lplMem
    std::stringstream payloadStream(FLAGS_cdb_payload);
    int payloadByte;
    while (payloadStream >> payloadByte) {
      lplMem.push_back(payloadByte & 0xff);
    }
  }

  // Create CDB block, set page to 0x9f for CDB command and set the default
  // password to unlock CDB functions
  CdbCommandBlock cdbBlock;
  cdbBlock.createCdbCmdGeneric(commandCode, lplMem);

  cdbBlock.selectCdbPage(bus, module);
  cdbBlock.setMsaPassword(bus, module, FLAGS_msa_password);

  // Run the CDB command and get the output
  bool rc = cdbBlock.cmisRunCdbCommand(bus, module);
  printf("CDB command %s\n", rc ? "Successful" : "Failed");

  uint8_t *pResp;
  uint8_t respLen = cdbBlock.getResponseData(&pResp);
  printf("Response data length %d", respLen);
  for (int i = 0; i < respLen; i++) {
    if (i % 16 == 0) {
      printf("\n%.2x: ", 136 + i);
    } else if (i % 8 == 0) {
      printf(" ");
    }
    printf("%.2x ", pResp[i]);
  }
  printf("\n");
}

/*
 * printVdmInfo
 *
 * Read the VDM information and print. The VDM config is present in page 20
 * (2 bytes per config entry) and the VDM values are present in page 24 (2
 * bytes value).
 */
bool printVdmInfo(TransceiverI2CApi* bus, unsigned int port) {
  DOMDataUnion domDataUnion;

  printf("Displaying VDM info for module %d:\n", port);

  if (!FLAGS_direct_i2c) {
    // Read the optics data from qsfp_service
    folly::EventBase evb;

    int32_t idx = port - 1;
    auto domDataUnionMap = fetchDataFromQsfpService({idx}, evb);
    auto iter = domDataUnionMap.find(idx);
    if(iter == domDataUnionMap.end()) {
        fprintf(stderr, "Port %d is not present in QsfpService data\n", idx + 1);
        return false;
    } else {
      domDataUnion = iter->second;
    }
  } else {
    // Read the optics data directly from this process
    try {
      domDataUnion = fetchDataFromLocalI2CBus(bus, port);
    } catch (const std::exception& ex) {
      fprintf(stderr, "error reading QSFP data %u: %s\n", port, ex.what());
      return false;
    }
  }

  if (domDataUnion.getType() != DOMDataUnion::Type::cmis) {
    printf("The VDM feature is available for CMIS optics only\n");
    return false;
  }

  CmisData cmisData = domDataUnion.get_cmis();
  auto page01Buf = can_throw(cmisData.page01_ref())->data();
  auto page20Buf = can_throw(cmisData.page20_ref())->data();
  auto page24Buf = can_throw(cmisData.page24_ref())->data();
  auto page21Buf = can_throw(cmisData.page21_ref())->data();
  auto page25Buf = can_throw(cmisData.page25_ref())->data();

  // Check for VDM presence first
  if (!(page01Buf[14] & 0x40)) {
    fprintf(stderr, "VDM not supported on this module %d\n", port);
    return false;
  }

  // Go over the list. Page 0x20/0x21 contains the config and page 0x24/0x25
  // contains the values

  auto findAndPrintVdmInfo = [] (const uint8_t* controlPageBuf, const uint8_t* dataPageBuf) {
    for (int i = 0; i < 127; i += 2) {
      uint8_t byteMsb = controlPageBuf[i];
      uint8_t byteLsb = controlPageBuf[i+1];

      if (!byteMsb && !byteLsb) {
        // No config present, continue to next
        continue;
      }

      uint8_t laneId = byteMsb & 0x0f;

      // Get the config info
      auto confType = VdmConfigTypeKey(byteLsb);
      auto vdmConfIt = vdmConfigTypeMap.find(confType);
      if (vdmConfIt == vdmConfigTypeMap.end()) {
        continue;
      }

      auto vdmConfigInfo = vdmConfIt->second;
      if (laneId > 7) {
        printf("Module %s = ", vdmConfigInfo.description.c_str());
      } else {
        printf ("Lane %d %s = ", laneId, vdmConfigInfo.description.c_str());
      }

      // Get the VDM value corresponding to config type and convert it appropriately.
      // The U16 value will be = byte0 + byte1 * lsbScale
      // For F16 value:
      //   exponent = (byte0 >> 3) - 24
      //   mantissa = (byte0 & 0x7) << 8 | byte1
      //   value = mantissa * 10^exponent
      double vdmValue;
      if (vdmConfigInfo.dataType == VDM_DATA_TYPE_U16) {
        vdmValue = dataPageBuf[i] + (dataPageBuf[i+1] * vdmConfigInfo.lsbScale);
        printf("%.2f %s\n", vdmValue, vdmConfigInfo.unit.c_str());
      } else if (vdmConfigInfo.dataType == VDM_DATA_TYPE_F16) {
        int expon = dataPageBuf[i] >> 3;
        int mant = ((dataPageBuf[i] & 0x7) << 8) | dataPageBuf[i+1];
        expon -= 24;
        vdmValue = mant * exp10(expon);
        printf("%.2e %s\n", vdmValue, vdmConfigInfo.unit.c_str());
      } else {
        printf("Unknown format\n");
      }
    }
  };

  // Media side info: control page = 0x20, data page = 0x24
  findAndPrintVdmInfo(page20Buf, page24Buf);
  // Host side info: control page = 0x20, data page = 0x24
  findAndPrintVdmInfo(page21Buf, page25Buf);

  return true;
}

} // namespace facebook::fboss
