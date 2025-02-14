// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/util/wedge_qsfp_util.h"

#include "fboss/lib/usb/GalaxyI2CBus.h"
#include "fboss/lib/usb/WedgeI2CBus.h"

#include "fboss/qsfp_service/module/QsfpModule.h"
#include "fboss/qsfp_service/module/QsfpUtil.h"
#include "fboss/qsfp_service/module/cmis/CmisModule.h"
#include "fboss/qsfp_service/module/sff/SffModule.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeQsfp.h"

#include "fboss/qsfp_service/lib/QsfpClient.h"
#include "fboss/qsfp_service/module/cmis/CmisFieldInfo.h"

#include "fboss/util/qsfp/QsfpServiceDetector.h"
#include "fboss/util/qsfp/QsfpUtilContainer.h"

#include <folly/Conv.h>
#include <folly/Exception.h>
#include <folly/FileUtil.h>
#include <folly/Memory.h>
#include <folly/io/async/EventBase.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <chrono>
#include "fboss/agent/EnumUtils.h"

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sysexits.h>
#include <fstream>

#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

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

std::string getStateNameString(
    uint8_t stateCode,
    const std::map<uint8_t, std::string>& nameMap) {
  std::string stateName = "UNKNOWN";
  if (auto iter = nameMap.find(stateCode); iter != nameMap.end()) {
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
  VDM_CONFIG_PAM4_LEVEL0_SD_LINE = 100,
  VDM_CONFIG_PAM4_LEVEL1_SD_LINE = 101,
  VDM_CONFIG_PAM4_LEVEL2_SD_LINE = 102,
  VDM_CONFIG_PAM4_LEVEL3_SD_LINE = 103,
  VDM_CONFIG_PAM4_MPI_LINE = 104,
};

struct VdmConfigType {
  std::string description;
  VdmDataType dataType;
  float lsbScale;
  std::string unit;
};

// clang-format off
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
  {VDM_CONFIG_ERR_FRAMES_MEDIA_IN_AVG, {"Err Frames Media Input Accumulated", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_MEDIA_IN_CURR, {"Err Frames Media Input Current", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PAM4_LEVEL0_SD_LINE, {"PAM4 Level0 Standard Deviation Line", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PAM4_LEVEL1_SD_LINE, {"PAM4 Level1 Standard Deviation Line", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PAM4_LEVEL2_SD_LINE, {"PAM4 Level2 Standard Deviation Line", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PAM4_LEVEL3_SD_LINE, {"PAM4 Level3 Standard Deviation Line", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PAM4_MPI_LINE, {"PAM4 MPI Line", VDM_DATA_TYPE_F16, 0, ""}},
  // Host side
  {VDM_CONFIG_ESNR_HOST_INPUT, {"eSNR Host Input", VDM_DATA_TYPE_U16, 1.0/256.0, "dB"}},
  {VDM_CONFIG_PAM4_LTP_HOST_INPUT, {"PAM4 LTP Host Input", VDM_DATA_TYPE_U16, 1.0/256.0, "dB"}},
  {VDM_CONFIG_PRE_FEC_BER_HOST_IN_MIN, {"Pre-FEC BER Host Input Minimum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PRE_FEC_BER_HOST_IN_MAX, {"Pre-FEC BER Host Input Maximum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PRE_FEC_BER_HOST_IN_AVG, {"Pre-FEC BER Host Input Average", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_PRE_FEC_BER_HOST_IN_CURR, {"Pre-FEC BER Host Input Current", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_HOST_IN_MIN, {"Err Frames Host Input Minimum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_HOST_IN_MAX, {"Err Frames Host Input Maximum", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_HOST_IN_AVG, {"Err Frames Host Input Accumulated", VDM_DATA_TYPE_F16, 0, ""}},
  {VDM_CONFIG_ERR_FRAMES_HOST_IN_CURR, {"Err Frames Host Input Current", VDM_DATA_TYPE_F16, 0, ""}},
};
// clang-format on
} // namespace

using apache::thrift::can_throw;
using folly::MutableByteRange;
using folly::StringPiece;
using std::make_pair;
using std::pair;
using std::chrono::seconds;
using std::chrono::steady_clock;

// We can check on the hardware type:

const char* chipCheckPath = "/sys/bus/pci/devices/0000:01:00.0/device";
const char* trident2 = "0xb850\n"; // Note expected carriage return
static constexpr uint16_t hexBase = 16;
static constexpr uint16_t decimalBase = 10;
static constexpr uint16_t eePromDefault = 255;
static constexpr uint16_t maxGauge = 30;

DEFINE_bool(
    clear_low_power,
    false,
    "Allow the QSFP to use higher power; needed for LR4 optics");
DEFINE_bool(
    set_low_power,
    false,
    "Force the QSFP to limit power usage; Only useful for testing");
DEFINE_bool(
    tx_disable,
    false,
    "Set the TX disable bits, use with --channel optionally");
DEFINE_bool(
    tx_enable,
    false,
    "Clear the TX disable bits, use with --channel optionally");
DEFINE_int32(channel, -1, "Channel id within the module (starts from 1)");
DEFINE_bool(set_40g, false, "Rate select 40G, for CWDM4 modules.");
DEFINE_bool(set_100g, false, "Rate select 100G, for CWDM4 modules.");
DEFINE_int32(
    app_sel,
    0,
    "Select application, for CMIS modules. In 200G modules 1 is for 200G and 2 is for 100G");
DEFINE_bool(cdr_enable, false, "Set the CDR bits if transceiver supports it");
DEFINE_bool(
    cdr_disable,
    false,
    "Clear the CDR bits if transceiver supports it");
DEFINE_int32(open_timeout, 30, "Number of seconds to wait to open bus");
DEFINE_bool(
    direct_i2c,
    false,
    "Read Transceiver info from i2c bus instead of qsfp_service");
DEFINE_bool(qsfp_hard_reset, false, "Issue a hard reset to port QSFP");
DEFINE_bool(
    qsfp_reset,
    false,
    "Issue reset to QSFP ports. Will go through qsfp_service unless direct_i2c is specified.");
DEFINE_string(
    qsfp_reset_type,
    "HARD_RESET",
    "Reset via qsfp_service. Options supported are HARD_RESET");
DEFINE_string(
    qsfp_reset_action,
    "RESET_THEN_CLEAR",
    "Reset via qsfp_service. Options supported are RESET_THEN_CLEAR");
DEFINE_bool(
    electrical_loopback,
    false,
    "Set the module to be electrical loopback, only for Miniphoton");
DEFINE_bool(
    optical_loopback,
    false,
    "Set the module to be optical loopback, only for Miniphoton");
DEFINE_bool(
    clear_loopback,
    false,
    "Clear the module loopback bits, only for Miniphoton");
DEFINE_bool(skip_check, false, "Skip checks for setting module loopback");
DEFINE_bool(
    read_reg,
    false,
    "Read a register, use with --offset and optionally --length");
DEFINE_bool(write_reg, false, "Write a register, use with --offset and --data");
DEFINE_int32(offset, -1, "The offset of register to read/write (0..255)");
DEFINE_string(
    data,
    "",
    "Comma-separated list of bytes to write to the register, use with --offset");
DEFINE_int32(
    length,
    1,
    "The number of bytes to read from the register (1..128), use with --offset");
DEFINE_int32(
    page,
    -1,
    "The page to use when doing transceiver register access");
DEFINE_int32(
    pause_remediation,
    0,
    "Number of seconds to prevent qsfp_service from doing remediation to modules");
DEFINE_bool(
    get_remediation_until_time,
    false,
    "Get the local time that current remediationUntil time sets to");
DEFINE_bool(
    update_module_firmware,
    false,
    "Update firmware for module, use with --firmware_filename");
DEFINE_string(
    firmware_filename,
    "",
    "Module firmware filename along with path");
DEFINE_uint32(
    msa_password,
    0x00001011,
    "MSA password for module privilige operation");
DEFINE_uint32(image_header_len, 0, "Firmware image header length");
DEFINE_string(
    forced_module_type,
    "",
    "Forced module type, values are cmis or sff");
DEFINE_bool(
    get_module_fw_info,
    false,
    "Get the module firmware info for list of ports, use with portA and portB");
DEFINE_bool(cdb_command, false, "Read from CDB block, use with --command_code");
DEFINE_int32(command_code, -1, "CDB command code (16 bit)");
DEFINE_string(cdb_payload, "", "CDB command LPL payload (list of bytes)");
DEFINE_bool(
    update_bulk_module_fw,
    false,
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
DEFINE_bool(
    batch_ops,
    false,
    "Do batch read/write operations on module, use with --batchfile and --direct_i2c");
DEFINE_string(
    batchfile,
    "",
    "Batch file for bulk read/write operations, Format: OP(R/W) Offset Value DelayMS");
DEFINE_uint32(i2c_address, 0x50, "i2c address");
DEFINE_bool(
    prbs_start,
    false,
    "Start the PRBS on a module line side, use with --generator or --checker");
DEFINE_bool(
    prbs_stop,
    false,
    "Stop the PRBS on a module line side, use with --generator or --checker");
DEFINE_string(prbs_pattern, "PRBS31Q", "PRBS polynominal, default is PRBS31Q");
DEFINE_bool(prbs_stats, false, "Get the PRBS stats from a module line side");
DEFINE_bool(generator, false, "Start or Stop PRBS Generator side");
DEFINE_bool(checker, false, "Start or Stop PRBS Checker side");
DEFINE_bool(
    module_io_stats,
    false,
    "Get the Module read/write transaction stats");
DEFINE_bool(
    capabilities,
    false,
    "Show module capabilities for all present modules");
DEFINE_bool(
    dump_tcvr_i2c_log,
    false,
    "Dump the transceiver i2c log to /dev/shm/fboss/qsfp_service");

namespace {
struct ModulePartInfo_s {
  std::array<uint8_t, 16> partNo;
  uint32_t headerLen;
};
// clang-format off
struct ModulePartInfo_s modulePartInfo[] = {
  // Finisar 200G module info
  {{'F','T','C','C','1','1','1','2','E','1','P','L','L','-','F','B'}, 64},
  // Finisar 400G module info
  {{'F','T','C','D','4','3','1','3','E','2','P','C','L',0x20,0x20,0x20}, 64},
  // Finisar 400G module info
  {{'F','T','C','D','4','3','1','3','E','2','P','C','L','-','F','B'}, 64},
  // Finisar 400G module info
  {{'F','T','C','D','4','3','1','3','E','2','P','C','L','F','B','1'}, 64},
  // Finisar 400G module info
  {{'F','T','C','D','4','3','1','3','E','2','P','C','L','F','B','2'}, 64},
  // Finisar 400G module info
  {{'F','T','C','D','4','3','1','3','E','2','P','C','L','F','B','3'}, 64},
  // Finisar 400G module info
  {{'F','T','C','D','4','3','1','3','E','2','P','C','L','F','B','4'}, 64},
  // Finisar 400G LR4 module info
  {{'F','T','C','D','4','3','2','3','E','2','P','C','L',0x20,0x20,0x20}, 64},
  // Innolight 200G module info
  {{'T','-','F','X','4','F','N','T','-','H','F','B',0x20,0x20,0x20,0x20}, 48},
  // Innolight 200G module info
  {{'T','-','F','X','4','F','N','T','-','H','F','P',0x20,0x20,0x20,0x20}, 48},
  // Innolight 200G module info
  {{'T','-','F','X','4','F','N','T','-','H','F','S',0x20,0x20,0x20,0x20}, 48},
  // Innolight 400G module info
  {{'T','-','D','Q','4','C','N','T','-','N','F','B',0x20,0x20,0x20,0x20}, 48},
  // Innolight 400G module info
  {{'T','-','D','Q','4','C','N','T','-','N','F','2',0x20,0x20,0x20,0x20}, 48},
  // Innolight 400G module info
  {{'T','-','D','Q','4','C','N','T','-','N','F','M',0x20,0x20,0x20,0x20}, 48},
  // Intel 200G module info
  {{'S','P','T','S','M','P','3','C','L','C','K','8',0x20,0x20,0x20,0x20}, 48},
  // Intel 200G module info
  {{'S','P','T','S','M','P','3','C','L','C','K','9',0x20,0x20,0x20,0x20}, 48},
  // Intel 400G module info
  {{'S','P','T','S','H','P','3','C','L','C','K','S',0x20,0x20,0x20,0x20}, 48}
};
// clang-format on
constexpr uint8_t kNumModuleInfo =
    sizeof(modulePartInfo) / sizeof(struct ModulePartInfo_s);
}; // namespace

namespace facebook::fboss {

std::map<prbs::PrbsPolynomial, int> cmisPrbsPolynominalMap = {
    {prbs::PrbsPolynomial::PRBS31Q, 0},
    {prbs::PrbsPolynomial::PRBS31, 1},
    {prbs::PrbsPolynomial::PRBS23Q, 2},
    {prbs::PrbsPolynomial::PRBS23, 3},
    {prbs::PrbsPolynomial::PRBS15Q, 4},
    {prbs::PrbsPolynomial::PRBS15, 5},
    {prbs::PrbsPolynomial::PRBS13Q, 6},
    {prbs::PrbsPolynomial::PRBS13, 7},
    {prbs::PrbsPolynomial::PRBS9Q, 8},
    {prbs::PrbsPolynomial::PRBS9, 9},
    {prbs::PrbsPolynomial::PRBS7Q, 10},
    {prbs::PrbsPolynomial::PRBS7, 11}};

// Forward declaration of utility functions for firmware upgrade
std::vector<unsigned int> getUpgradeModList(
    DirectI2cInfo i2cInfo,
    std::vector<unsigned int> portlist,
    std::string moduleType,
    std::string fwVer);

void fwUpgradeThreadHandler(
    DirectI2cInfo i2cInfo,
    std::vector<unsigned int> modlist,
    std::string firmwareFilename,
    uint32_t imageHdrLen);

void setModulePrbsViaService(
    folly::EventBase& evb,
    std::vector<std::string> portList,
    bool start);
void setModulePrbsDirect(
    DirectI2cInfo i2cInfo,
    std::vector<std::string> portList,
    bool start);
void setModulePrbsDirectCmis(TransceiverI2CApi* bus, int module, bool start);
void getModulePrbsStatsViaService(
    folly::EventBase& evb,
    std::vector<PortID> portList);
void getModulePrbsStatsDirect(
    DirectI2cInfo i2cInfo,
    const std::vector<PortID>& portList);
void getModulePrbsStatsDirectCmis(TransceiverI2CApi* bus, int module);

std::ostream& operator<<(std::ostream& os, const FlagCommand& cmd) {
  gflags::CommandLineFlagInfo flagInfo;

  os << "Command:\n"
     << gflags::DescribeOneFlag(
            gflags::GetCommandLineFlagInfoOrDie(cmd.command.c_str()));
  os << "Flags:\n";
  for (auto flag : cmd.flags) {
    os << gflags::DescribeOneFlag(
        gflags::GetCommandLineFlagInfoOrDie(flag.c_str()));
  }

  return os;
}

std::unique_ptr<facebook::fboss::QsfpServiceAsyncClient> getQsfpClient(
    folly::EventBase& evb) {
  return std::move(QsfpClient::createClient(&evb)).getVia(&evb);
}

/*
 * This function returns the transceiver management interface through either
 * qsfp_service query or direct_i2c operation. This function takes 1 based
 * based module Ids and returns the map of 1 based module Ids to the
 * corresponding management interface types
 */
std::map<int32_t, TransceiverManagementInterface> getModuleType(
    const std::vector<unsigned int>& ports) {
  std::map<int32_t, TransceiverManagementInterface> moduleTypeMap;

  if (QsfpServiceDetector::getInstance()->isQsfpServiceActive()) {
    folly::EventBase& evb = QsfpUtilContainer::getInstance()->getEventBase();
    std::map<int32_t, TransceiverManagementInterface> moduleTypeMapZeroBased;
    moduleTypeMapZeroBased = getModuleTypeViaService(ports, evb);

    // Service returns 0 based module id in result. Change it to 1 based to
    // make it compatible with input values
    for (auto& modType : moduleTypeMapZeroBased) {
      moduleTypeMap[modType.first + 1] = modType.second;
    }
  } else {
    TransceiverI2CApi* bus =
        QsfpUtilContainer::getInstance()->getTransceiverBus();
    for (auto port : ports) {
      moduleTypeMap[port] = getModuleTypeDirect(bus, port);
    }
  }
  return moduleTypeMap;
}

/*
 * This function returns the transceiver management interface
 * by reading the register 0 directly from module
 */
TransceiverManagementInterface getModuleTypeDirect(
    TransceiverI2CApi* bus,
    unsigned int port) {
  uint8_t moduleId = static_cast<uint8_t>(TransceiverModuleIdentifier::UNKNOWN);

  // Get the module id to differentiate between CMIS (0x1e) and SFF
  for (auto retry = 0; retry < numRetryGetModuleType; retry++) {
    try {
      bus->moduleRead(
          port, {TransceiverAccessParameter::ADDR_QSFP, 0, 1}, &moduleId);
    } catch (const I2cError&) {
      fprintf(
          stderr, "QSFP %d: not present or read error, retrying...\n", port);
    }
  }

  return QsfpModule::getTransceiverManagementInterface(moduleId, port);
}

/*
 * This function returns the transceiver management interfaces
 * by reading the register 0 indirectly from modules. If there is an error,
 * this function returns an empty interface map.
 */
std::map<int32_t, TransceiverManagementInterface> getModuleTypeViaService(
    const std::vector<unsigned int>& ports,
    folly::EventBase& evb) {
  std::map<int32_t, TransceiverManagementInterface> moduleTypes;
  const int offset = 0;
  const int length = 1;
  const int page = 0;
  std::vector<int32_t> idx = zeroBasedPortIds(ports);
  std::map<int32_t, ReadResponse> readResp;
  doReadRegViaService(idx, offset, length, page, evb, readResp);

  if (readResp.empty()) {
    XLOG(ERR) << "Indirect read error in getting module type";
    return moduleTypes;
  }

  for (const auto& response : readResp) {
    const auto moduleId = *(response.second.data()->data());
    const TransceiverManagementInterface modType =
        QsfpModule::getTransceiverManagementInterface(
            moduleId, response.first + 1);

    if (modType == TransceiverManagementInterface::NONE ||
        modType == TransceiverManagementInterface::UNKNOWN) {
      return std::map<int32_t, TransceiverManagementInterface>();
    } else {
      moduleTypes[response.first] = modType;
    }
  }

  return moduleTypes;
}

bool flipModuleUpperPage(
    TransceiverI2CApi* bus,
    unsigned int port,
    uint8_t page) {
  try {
    bus->moduleWrite(
        port,
        {TransceiverAccessParameter::ADDR_QSFP, 127, sizeof(page)},
        &page);
  } catch (const I2cError&) {
    fprintf(stderr, "QSFP %d: Fail to flip module upper page\n", port);
    return false;
  }
  return true;
}

bool overrideLowPower(unsigned int port, bool lowPower) {
  TransceiverManagementInterface managementInterface =
      TransceiverManagementInterface::NONE;

  if (QsfpServiceDetector::getInstance()->isQsfpServiceActive()) {
    folly::EventBase& evb = QsfpUtilContainer::getInstance()->getEventBase();
    std::map<int32_t, TransceiverManagementInterface> moduleTypes =
        getModuleTypeViaService({port}, evb);

    if (moduleTypes.empty()) {
      XLOG(ERR) << "Error in getting module type";
      return false;
    }

    for (auto [portIdx, moduleType] : moduleTypes) {
      managementInterface = moduleType;
    }
  } else {
    TransceiverI2CApi* bus =
        QsfpUtilContainer::getInstance()->getTransceiverBus();

    managementInterface = getModuleTypeDirect(bus, port);
  }

  uint8_t buf;

  if (managementInterface == TransceiverManagementInterface::CMIS) {
    readRegister(port, 26, 1, 0, &buf);
    lowPower ? buf |= kCmisPowerModeMask : buf &= ~kCmisPowerModeMask;
    writeRegister({port}, 26, 0, buf);
  } else if (managementInterface == TransceiverManagementInterface::SFF) {
    buf = {lowPower ? kSffLowPowerMode : kSffHighPowerMode};
    writeRegister({port}, 93, 0, buf);
  } else {
    fprintf(stderr, "QSFP %d: Unrecognizable management interface\n", port);
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
    bus->moduleWrite(
        port, {TransceiverAccessParameter::ADDR_QSFP, 127, 1}, &page0);

    bus->moduleRead(
        port, {TransceiverAccessParameter::ADDR_QSFP, 129, 1}, supported);
  } catch (const I2cError& ex) {
    fprintf(
        stderr,
        "Port %d: Unable to determine whether CDR supported: %s\n",
        port,
        ex.what());
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
    bus->moduleWrite(port, {TransceiverAccessParameter::ADDR_QSFP, 98, 1}, buf);
  } catch (const I2cError&) {
    fprintf(stderr, "QSFP %d: Failed to set CDR\n", port);
    return false;
  }
  return true;
}

bool rateSelect(unsigned int port, uint8_t value) {
  // If v1 is used, both at 10, if v2
  // 0b10 - 25G channels
  // 0b00 - 10G channels
  uint8_t version;
  uint8_t buf;

  readRegister(port, 141, 1, 0, &version);

  if (version & 1) {
    buf = 0b10;
  } else {
    buf = value;
  }

  writeRegister({port}, 87, 0, buf);
  writeRegister({port}, 88, 0, buf);

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
          {TransceiverAccessParameter::ADDR_QSFP,
           kCMISOffsetAppSelLane1 + channel,
           sizeof(applicationCode)},
          &applicationCode);
    }

    uint8_t applySet0 = 0x0f;
    bus->moduleWrite(
        port,
        {TransceiverAccessParameter::ADDR_QSFP,
         kCMISOffsetStageCtrlSet0,
         sizeof(applySet0)},
        &applySet0);
  } catch (const I2cError&) {
    // This generally means the QSFP module is not present.
    fprintf(stderr, "QSFP %d: fail to change application\n", port);
    return false;
  }
  return true;
}

std::vector<int32_t> zeroBasedPortIds(const std::vector<unsigned int>& ports) {
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
    const std::vector<int32_t>& ports,
    int offset,
    int length,
    int page,
    folly::EventBase& evb,
    std::map<int32_t, ReadResponse>& response) {
  auto client = getQsfpClient(evb);
  ReadRequest request;
  TransceiverIOParameters param;
  request.ids() = ports;
  param.offset() = offset;
  param.length() = length;
  if (page != -1) {
    param.page() = page;
  }
  request.parameter() = param;

  try {
    client->sync_readTransceiverRegister(response, request);
    for (const auto& iterator : response) {
      printf("Port Id : %d\n", iterator.first + 1);
      auto data = iterator.second.data()->data();
      if (iterator.second.data()->length() < length) {
        fprintf(stderr, "QSFP %d: fail to read module\n", iterator.first + 1);
        return;
      } else {
        displayReadRegData(data, offset, iterator.second.data()->length());
      }
    }
  } catch (const std::exception& ex) {
    fprintf(
        stderr, "error reading register from qsfp_service: %s\n", ex.what());
  }
}

void setPageDirect(TransceiverI2CApi* bus, unsigned int port, uint8_t page) {
  // Write the pageID in byte 127 of the lower page
  const int offset = 127;
  bus->moduleWrite(
      port, {static_cast<uint8_t>(FLAGS_i2c_address), offset, 1}, &page);
  printf("QSFP %d: successfully set pageID %d.\n", port, page);
}

void doReadRegDirect(
    TransceiverI2CApi* bus,
    unsigned int port,
    int offset,
    int length,
    int page) {
  std::array<uint8_t, 128> buf;
  try {
    if (page != -1) {
      setPageDirect(bus, port, page);
    }
    bus->moduleRead(
        port,
        {static_cast<uint8_t>(FLAGS_i2c_address), offset, length},
        buf.data());
  } catch (const I2cError&) {
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
      doReadRegDirect(bus, portNum, offset, length, page);
    }
  } else {
    // Release the bus access for QSFP service
    bus->close();
    std::vector<int32_t> idx = zeroBasedPortIds(ports);
    std::map<int32_t, ReadResponse> resp;
    doReadRegViaService(idx, offset, length, page, evb, resp);
  }
  return EX_OK;
}

void doWriteRegDirect(
    TransceiverI2CApi* bus,
    unsigned int port,
    int offset,
    int page,
    const std::vector<uint8_t>& data) {
  try {
    if (page != -1) {
      setPageDirect(bus, port, page);
    }
    bus->moduleWrite(
        port,
        {static_cast<uint8_t>(FLAGS_i2c_address),
         offset,
         static_cast<int>(data.size())},
        data.data());
  } catch (const I2cError&) {
    fprintf(stderr, "QSFP %d: not present or unwritable\n", port);
    return;
  }
  printf(
      "QSFP %d: successfully wrote %d bytes to %d.\n",
      port,
      static_cast<int>(data.size()),
      offset);
}

bool doWriteRegViaService(
    const std::vector<int32_t>& ports,
    int offset,
    int page,
    const std::vector<uint8_t>& data,
    folly::EventBase& evb) {
  bool retVal = true;
  auto client = getQsfpClient(evb);
  WriteRequest request;
  TransceiverIOParameters param;
  request.ids() = ports;
  param.offset() = offset;
  if (page != -1) {
    param.page() = page;
  }
  param.length() = data.size();
  request.parameter() = param;
  if (!data.size()) {
    fprintf(
        stderr,
        "QSFP %s: Fail to write register. No data specified.\n",
        folly::join(",", ports).c_str());
    return EX_SOFTWARE;
  }
  request.data() = data.front();
  // Thrift only allows signed types – temporarily converting to int8_t.
  request.bytes() = std::vector<int8_t>(data.begin(), data.end());
  std::map<int32_t, WriteResponse> response;

  try {
    client->sync_writeTransceiverRegister(response, request);
    for (const auto& iterator : response) {
      if (*(response[iterator.first].success())) {
        printf(
            "QSFP %d: successfully wrote %d bytes to %d.\n",
            iterator.first + 1,
            static_cast<int>(data.size()),
            offset);
      } else {
        retVal = false;
        fprintf(
            stderr, "QSFP %d: not present or unwritable\n", iterator.first + 1);
      }
    }
  } catch (const std::exception& ex) {
    fprintf(stderr, "error writing register via qsfp_service: %s\n", ex.what());
    retVal = false;
  }

  return retVal;
}

void readRegisterDirect(
    TransceiverI2CApi* bus,
    unsigned int port,
    int offset,
    int length,
    int page,
    uint8_t* buf) {
  try {
    if (page != -1) {
      setPageDirect(bus, port, page);
    }

    bus->moduleRead(
        port, {static_cast<uint8_t>(FLAGS_i2c_address), offset, length}, buf);
  } catch (const I2cError&) {
    fprintf(stderr, "QSFP %d: fail to read module\n", port);
  }
}

int readRegister(
    unsigned int port,
    int offset,
    int length,
    int page,
    uint8_t* buf) {
  if (offset == -1) {
    fprintf(
        stderr,
        "QSFP %d: Fail to read register. Specify offset using --offset\n",
        port);
    return EX_SOFTWARE;
  }

  if (length > 128) {
    fprintf(
        stderr,
        "QSFP %d: Fail to read register. The --length value should be between 1 to 128\n",
        port);
    return EX_SOFTWARE;
  }

  if (QsfpServiceDetector::getInstance()->isQsfpServiceActive()) {
    folly::EventBase& evb = QsfpUtilContainer::getInstance()->getEventBase();
    std::vector<int32_t> portIdx = zeroBasedPortIds({port});

    std::map<int32_t, ReadResponse> readResp;
    doReadRegViaService(portIdx, offset, length, page, evb, readResp);

    if (readResp.empty()) {
      fprintf(stderr, "QSFP %d: indirect read error", port);
      return EX_SOFTWARE;
    }

    if (readResp.size() != 1) {
      fprintf(
          stderr,
          "Got %lu responses for the read but expected 1",
          readResp.size());
      return EX_SOFTWARE;
    }

    auto dataPtr = readResp.begin()->second.data()->data();
    // The underlying buffer in ReadResponse will get cleaned up when it goes
    // out of scope, so copy the data to the buffer the user requested
    memcpy(buf, dataPtr, length);
  } else {
    TransceiverI2CApi* bus =
        QsfpUtilContainer::getInstance()->getTransceiverBus();
    readRegisterDirect(bus, port, offset, length, page, buf);
  }

  return EX_OK;
}

int doWriteReg(
    TransceiverI2CApi* bus,
    std::vector<unsigned int>& ports,
    int offset,
    int page,
    const std::vector<uint8_t>& data,
    folly::EventBase& evb) {
  if (offset == -1) {
    fprintf(
        stderr,
        "QSFP %s: Fail to write register. Specify offset using --offset\n",
        folly::join(",", ports).c_str());
    return EX_SOFTWARE;
  }

  if (offset < 0 || offset + data.size() > 256) {
    fprintf(
        stderr,
        "QSFP %s: Fail to write register. Fix --offset and --data length to be within a single page.\n",
        folly::join(",", ports).c_str());
    return EX_SOFTWARE;
  } else if (offset < 128 && offset + data.size() > 128) {
    fprintf(
        stderr,
        "QSFP %s: Fail to write register. Fix --offset and --data length to not perform a cross-page transaction.\n",
        folly::join(",", ports).c_str());
    return EX_SOFTWARE;
  }

  if (FLAGS_direct_i2c) {
    for (unsigned int portNum : ports) {
      doWriteRegDirect(bus, portNum, offset, page, data);
    }
  } else {
    // Release the bus access for QSFP service
    bus->close();
    std::vector<int32_t> idx = zeroBasedPortIds(ports);
    doWriteRegViaService(idx, offset, page, data, evb);
  }

  return EX_OK;
}

int writeRegister(
    const std::vector<unsigned int>& ports,
    int offset,
    int page,
    uint8_t data) {
  if (offset == -1) {
    fprintf(
        stderr,
        "QSFP %s: Fail to write register. Specify offset using --offset\n",
        folly::join(",", ports).c_str());
    return EX_SOFTWARE;
  }

  if (QsfpServiceDetector::getInstance()->isQsfpServiceActive()) {
    folly::EventBase& evb = QsfpUtilContainer::getInstance()->getEventBase();
    std::vector<int32_t> idx = zeroBasedPortIds(ports);
    doWriteRegViaService(idx, offset, page, {data}, evb);
  } else {
    TransceiverI2CApi* bus =
        QsfpUtilContainer::getInstance()->getTransceiverBus();

    for (unsigned int portNum : ports) {
      doWriteRegDirect(bus, portNum, offset, page, {data});
    }
  }

  return EX_OK;
}

/*
 * doBatchOps
 *
 * This function does the batch operation of read/write to a module. The batch
 * of instructions is provided by batchfile. The format of batchfile is below:
 *
 * OPCODE  REG_OFFSET  REG_VAL  DELAY_MS
 * W       127         0x0      10
 * R       148         0x0      5
 * R       149         0x0      5
 * R       150         0x0      5
 * R       151         0x0      5
 * W       127         0x13     10
 * R       130         0x0      5
 *
 * CLI format:
 *   wedge_qsfp_util <module> --batch_ops --batchfile <filename>
 * [--direct_i2c]
 */
int doBatchOps(
    TransceiverI2CApi* bus,
    std::vector<unsigned int>& ports,
    std::string batchfile,
    folly::EventBase& evb) {
  std::string fileContents;
  int rc, regAddr, regVal, delayMsec;
  char opCode;

  std::ifstream batchFileStream(batchfile.c_str());
  std::string commandLine;
  std::string headerLine;

  if (!getline(batchFileStream, commandLine)) {
    printf("Batchfile empty or read failure\n");
    return EX_USAGE;
  }

  while (getline(batchFileStream, commandLine)) {
    rc = std::sscanf(
        commandLine.c_str(),
        "%c %d %x %d",
        &opCode,
        &regAddr,
        &regVal,
        &delayMsec);
    if (rc != 4) {
      printf("Batchfile formating issue\n");
      return EX_USAGE;
    }

    // Print high resolution current time with command
    std::chrono::microseconds ms = duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch());
    std::time_t t = duration_cast<seconds>(ms).count();
    int fractional_seconds = ms.count() % 1000000;
    auto tm = std::tm{};
    localtime_r(&t, &tm);

    if (opCode == 'R') {
      printf(
          "%.2d:%.2d:%.2d.%.3d: Reading Reg offset %d, Postdelay %d msec\n",
          tm.tm_hour,
          tm.tm_min,
          tm.tm_sec,
          fractional_seconds,
          regAddr,
          delayMsec);
      doReadReg(bus, ports, regAddr, 1, -1, evb);
    } else {
      printf(
          "%.2d:%.2d:%.2d.%.3d: Writing Reg offset %d value 0x%x, Postdelay %d msec\n",
          tm.tm_hour,
          tm.tm_min,
          tm.tm_sec,
          fractional_seconds,
          regAddr,
          regVal,
          delayMsec);
      doWriteReg(bus, ports, regAddr, -1, {static_cast<uint8_t>(regVal)}, evb);
    }
    /* sleep override */
    usleep(delayMsec * 1000);
  }
  return EX_OK;
}

std::map<int32_t, DOMDataUnion> fetchDataFromQsfpService(
    const std::vector<int32_t>& ports,
    folly::EventBase& evb) {
  auto client = getQsfpClient(evb);

  std::map<int32_t, TransceiverInfo> qsfpInfoMap;

  client->sync_getTransceiverInfo(qsfpInfoMap, ports);

  std::vector<int32_t> presentPorts;
  for (auto& qsfpInfo : qsfpInfoMap) {
    if (*can_throw(qsfpInfo.second.tcvrState())->present()) {
      presentPorts.push_back(qsfpInfo.first);
    }
  }

  std::map<int32_t, DOMDataUnion> domDataUnionMap;

  if (!presentPorts.empty()) {
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

DOMDataUnion fetchDataFromLocalI2CBus(
    DirectI2cInfo i2cInfo,
    unsigned int port) {
  // port is 1 based and WedgeQsfp is 0 based.
  auto qsfpImpl = std::make_unique<WedgeQsfp>(
      port - 1, i2cInfo.bus, i2cInfo.transceiverManager, /*logBuffer*/ nullptr);
  auto mgmtInterface = qsfpImpl->getTransceiverManagementInterface();
  if (!FLAGS_forced_module_type.empty()) {
    if (FLAGS_forced_module_type == "cmis") {
      mgmtInterface = TransceiverManagementInterface::CMIS;
    } else if (FLAGS_forced_module_type == "sff") {
      mgmtInterface = TransceiverManagementInterface::SFF;
    } else {
      throw FbossError(
          "Invalid forced module type specified: ", FLAGS_forced_module_type);
    }
  }

  auto cfgPtr = i2cInfo.transceiverManager->getTransceiverConfig();

  // On these platforms, we are configuring the 200G optics in 2x50G
  // experimental mode. Thus 2 of the 4 lanes remain disabled which kicks in the
  // remediation logic and flaps the other 2 ports. Disabling remediation for
  // just these 2 platforms as this is an experimental mode only
  bool cmisSupportRemediate = true;
  auto tcvrMgr = i2cInfo.transceiverManager;
  if (tcvrMgr->getPlatformType() == PlatformType::PLATFORM_MERU400BIU ||
      tcvrMgr->getPlatformType() == PlatformType::PLATFORM_MERU400BFU) {
    cmisSupportRemediate = false;
  }

  auto tcvrID = TransceiverID(qsfpImpl->getNum());
  if (mgmtInterface == TransceiverManagementInterface::CMIS) {
    auto cmisModule = std::make_unique<CmisModule>(
        tcvrMgr->getPortNames(tcvrID),
        qsfpImpl.get(),
        cfgPtr,
        cmisSupportRemediate,
        tcvrMgr->getTransceiverName(tcvrID));
    try {
      cmisModule->refresh();
    } catch (FbossError&) {
      printf("refresh() FbossError for port %d\n", port);
    } catch (I2cError&) {
      printf("refresh() YampI2cError for port %d\n", port);
    }
    return cmisModule->getDOMDataUnion();
  } else if (mgmtInterface == TransceiverManagementInterface::SFF) {
    auto sffModule = std::make_unique<SffModule>(
        tcvrMgr->getPortNames(tcvrID),
        qsfpImpl.get(),
        cfgPtr,
        tcvrMgr->getTransceiverName(tcvrID));
    sffModule->refresh();
    return sffModule->getDOMDataUnion();
  } else {
    throw std::runtime_error(folly::sformat(
        "Unknown transceiver management interface: {}.",
        static_cast<int>(mgmtInterface)));
  }
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
  return asctime_r(&localtime_result, buf.data());
}

void setPauseRemediation(
    folly::EventBase& evb,
    std::vector<std::string> portList) {
  auto client = getQsfpClient(evb);
  client->sync_pauseRemediation(FLAGS_pause_remediation, portList);
  for (auto port : portList) {
    printf("PauseRemediation set for port %s\n", port.c_str());
  }
}

void doGetRemediationUntilTime(
    folly::EventBase& evb,
    std::vector<std::string> portList) {
  auto client = getQsfpClient(evb);
  std::map<std::string, int32_t> info;
  client->sync_getRemediationUntilTime(info, portList);
  for (auto infoItr : info) {
    printf(
        "RemediationUntil time for port %s is set to : %s",
        infoItr.first.c_str(),
        getLocalTime(infoItr.second).c_str());
  }
}

void printChannelMonitor(
    unsigned int index,
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
    printf(
        "    Channel %d:   %12fmW  %12fmW  %12fmA  %12f\n",
        index,
        rxPower,
        txPower,
        txBias,
        rxSNR.value());
  } else {
    printf(
        "    Channel %d:   %12fmW  %12fmW  %12fmA  %12s\n",
        index,
        rxPower,
        txPower,
        txBias,
        "N/A");
  }
}

uint8_t getSensorMonitors(const Sensor& sensor) {
  uint8_t value = 0;
  if (auto flags = sensor.flags()) {
    value = (*(flags->alarm()->high()) << 3) | (*(flags->alarm()->low()) << 2) |
        (*(flags->warn()->high()) << 1) | (*(flags->warn()->low()));
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
    printf("%-22s     %-12s", detail, "Lane 1");
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
  printf("    Temp: 0x%02x\n", getSensorMonitors(*(sensors.temp())));
  printf("    Vcc: 0x%02x\n", getSensorMonitors(*(sensors.vcc())));
}

void printChannelInterruptFlags(const std::vector<Channel>& channels) {
  int numChannels = channels.size();
  printLaneLine("  Lane Interrupts: ", numChannels);

  auto getHighAlarm = [](const Sensor& sensor) {
    return *(sensor.flags()->alarm()->high());
  };
  auto getHighWarn = [](const Sensor& sensor) {
    return *(sensor.flags()->warn()->high());
  };
  auto getLowAlarm = [](const Sensor& sensor) {
    return *(sensor.flags()->alarm()->low());
  };
  auto getLowWarn = [](const Sensor& sensor) {
    return *(sensor.flags()->warn()->low());
  };

  printf("\n    %-22s", "Tx Pwr High Alarm");
  for (const auto& channel : channels) {
    printf(" %-12d", getHighAlarm(*(channel.sensors()->txPwr())));
  }
  printf("\n    %-22s", "Tx Pwr High Warn");
  for (const auto& channel : channels) {
    printf(" %-12d", getHighWarn(*(channel.sensors()->txPwr())));
  }
  printf("\n    %-22s", "Tx Pwr Low Alarm");
  for (const auto& channel : channels) {
    printf(" %-12d", getLowAlarm(*(channel.sensors()->txPwr())));
  }
  printf("\n    %-22s", "Tx Pwr Low Warn");
  for (const auto& channel : channels) {
    printf(" %-12d", getLowWarn(*(channel.sensors()->txPwr())));
  }

  printf("\n    %-22s", "Tx Bias High Alarm");
  for (const auto& channel : channels) {
    printf(" %-12d", getHighAlarm(*(channel.sensors()->txBias())));
  }
  printf("\n    %-22s", "Tx Bias High Warn");
  for (const auto& channel : channels) {
    printf(" %-12d", getHighWarn(*(channel.sensors()->txBias())));
  }
  printf("\n    %-22s", "Tx Bias Low Alarm");
  for (const auto& channel : channels) {
    printf(" %-12d", getLowAlarm(*(channel.sensors()->txBias())));
  }
  printf("\n    %-22s", "Tx Bias Low Warn");
  for (const auto& channel : channels) {
    printf(" %-12d", getLowWarn(*(channel.sensors()->txBias())));
  }

  printf("\n    %-22s", "Rx Pwr High Alarm");
  for (const auto& channel : channels) {
    printf(" %-12d", getHighAlarm(*(channel.sensors()->rxPwr())));
  }
  printf("\n    %-22s", "Rx Pwr High Warn");
  for (const auto& channel : channels) {
    printf(" %-12d", getHighWarn(*(channel.sensors()->rxPwr())));
  }
  printf("\n    %-22s", "Rx Pwr Low Alarm");
  for (const auto& channel : channels) {
    printf(" %-12d", getLowAlarm(*(channel.sensors()->rxPwr())));
  }
  printf("\n    %-22s", "Rx Pwr Low Warn");
  for (const auto& channel : channels) {
    printf(" %-12d", getLowWarn(*(channel.sensors()->rxPwr())));
  }

  printf("\n");
}

void printHostLaneSignals(const std::vector<HostLaneSignals>& signals) {
  unsigned int numLanes = signals.size();
  if (numLanes == 0) {
    return;
  }
  printLaneLine("  Host Lane Signals: ", numLanes);
  // Assumption: If a signal is valid for first lane, it is also valid for
  // other lanes.
  if (signals[0].txLos()) {
    printf("\n    %-22s", "Tx LOS");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.txLos()));
    }
  }
  if (signals[0].txLol()) {
    printf("\n    %-22s", "Tx LOL");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.txLol()));
    }
  }
  if (signals[0].txAdaptEqFault()) {
    printf("\n    %-22s", "Tx Adaptive Eq Fault");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.txAdaptEqFault()));
    }
  }
  if (signals[0].dataPathDeInit()) {
    printf("\n    %-22s", "Datapath de-init");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.dataPathDeInit()));
    }
  }
  if (signals[0].cmisLaneState()) {
    printf("\n    %-22s", "Lane state");
    for (const auto& signal : signals) {
      printf(
          " %-12s",
          apache::thrift::util::enumNameSafe(*(signal.cmisLaneState()))
              .substr(0, 12)
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
  // Assumption: If a signal is valid for first lane, it is also valid for
  // other lanes.
  if (signals[0].rxLos()) {
    printf("\n    %-22s", "Rx LOS");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.rxLos()));
    }
  }
  if (signals[0].rxLol()) {
    printf("\n    %-22s", "Rx LOL");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.rxLol()));
    }
  }
  if (signals[0].txFault()) {
    printf("\n    %-22s", "Tx Fault");
    for (const auto& signal : signals) {
      printf(" %-12d", *(signal.txFault()));
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
  if (settings[0].txInputEqualization()) {
    printf("\n    %-22s", "Tx Input Equalization");
    for (const auto& setting : settings) {
      printf(" 0x%-10x", *(setting.txInputEqualization()));
    }
  }
  if (settings[0].rxOutputEmphasis()) {
    printf("\n    %-22s", "Rx Output Emphasis");
    for (const auto& setting : settings) {
      printf(" 0x%-10x", *(setting.rxOutputEmphasis()));
    }
  }
  if (settings[0].rxOutputPreCursor()) {
    printf("\n    %-22s", "Rx Out Precursor");
    for (const auto& setting : settings) {
      auto pre = *(setting.rxOutputPreCursor());
      printf(" %-12d", pre);
    }
  }
  if (settings[0].rxOutputPostCursor()) {
    printf("\n    %-22s", "Rx Out Postcursor");
    for (const auto& setting : settings) {
      auto post = *(setting.rxOutputPostCursor());
      printf(" %-12d", post);
    }
  }
  if (settings[0].rxOutputAmplitude()) {
    printf("\n    %-22s", "Rx Out Amplitude");
    for (const auto& setting : settings) {
      auto amp = *(setting.rxOutputAmplitude());
      printf(" %-12d", amp);
    }
  }
  if (settings[0].rxOutput()) {
    printf("\n    %-22s", "Rx Output Disable");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.rxOutput()));
    }
  }
  if (settings[0].rxSquelch()) {
    printf("\n    %-22s", "Rx Squelch Disable");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.rxSquelch()));
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
  if (settings[0].txDisable()) {
    printf("\n    %-22s", "Tx Disable");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.txDisable()));
    }
  }
  if (settings[0].txSquelch()) {
    printf("\n    %-22s", "Tx Squelch Disable");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.txSquelch()));
    }
  }
  if (settings[0].txAdaptiveEqControl()) {
    printf("\n    %-22s", "Tx Adaptive Eq Ctrl");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.txAdaptiveEqControl()));
    }
  }
  if (settings[0].txSquelchForce()) {
    printf("\n    %-22s", "Tx Forced Squelch");
    for (const auto& setting : settings) {
      printf(" %-12d", *(setting.txSquelchForce()));
    }
  }
  printf("\n");
}

void printGlobalDomMonitors(const GlobalSensors& sensors) {
  printf("  Global DOM Monitors:\n");
  printf("    Temperature: %.2f C\n", *(sensors.temp()->value()));
  printf("    Supply Voltage: %.2f V\n", *(sensors.vcc()->value()));
}

void printLaneDomMonitors(const std::vector<Channel>& channels) {
  if (channels.empty()) {
    return;
  }
  int numChannels = channels.size();
  printLaneLine("  Lane Dom Monitors:", numChannels);

  printf("\n    %-22s", "Tx Pwr (mW)");
  for (const auto& channel : channels) {
    printf(" %-12.2f", *(channel.sensors()->txPwr()->value()));
  }
  if (channels[0].sensors()->txPwrdBm()) {
    printf("\n    %-22s", "Tx Pwr (dBm)");
    for (const auto& channel : channels) {
      printf(" %-12.2f", *(channel.sensors()->txPwrdBm()->value()));
    }
  }
  printf("\n    %-22s", "Rx Pwr (mW)");
  for (const auto& channel : channels) {
    printf(" %-12.2f", *(channel.sensors()->rxPwr()->value()));
  }
  if (channels[0].sensors()->rxPwrdBm()) {
    printf("\n    %-22s", "Rx Pwr (dBm)");
    for (const auto& channel : channels) {
      printf(" %-12.2f", *(channel.sensors()->rxPwrdBm()->value()));
    }
  }
  printf("\n    %-22s", "Tx Bias (mA)");
  for (const auto& channel : channels) {
    printf(" %-12.2f", *(channel.sensors()->txBias()->value()));
  }
  if (auto rxSnrCh0 = channels[0].sensors()->rxSnr()) {
    printf("\n    %-22s", "Rx SNR");
    for (const auto& channel : channels) {
      if (auto snr = channel.sensors()->rxSnr()) {
        printf(" %-12.2f", *((*snr).value()));
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
        *(levels.alarm()->high()));
    printf(
        "  %-15s %-14s %.2f\n",
        monitor,
        "High Warning",
        *(levels.warn()->high()));
    printf(
        "  %-15s %-14s %.2f\n",
        monitor,
        "Low Warning",
        *(levels.warn()->low()));
    printf(
        "  %-15s %-14s %.2f\n", monitor, "Low Alarm", *(levels.alarm()->low()));
  };
  printMonitorThreshold("  Temp (C)", *(thresholds.temp()));
  printMonitorThreshold("  Vcc (V)", *(thresholds.vcc()));
  printMonitorThreshold("  Rx Power (mW)", *(thresholds.rxPwr()));
  printMonitorThreshold("  Tx Power (mW)", *(thresholds.txPwr()));
  printMonitorThreshold("  Tx Bias (mA)", *(thresholds.txBias()));
}

void printManagementInterface(
    const TransceiverInfo& transceiverInfo,
    const char* fmt) {
  const TcvrState& tcvrState = *can_throw(transceiverInfo.tcvrState());
  if (auto mgmtInterface = tcvrState.transceiverManagementInterface()) {
    printf(fmt, apache::thrift::util::enumNameSafe(*mgmtInterface).c_str());
  }
}

void printMediaInterfaceCode(MediaInterfaceCode media, const char* fmt) {
  printf(fmt, apache::thrift::util::enumNameSafe(media).c_str());
}

void printVerboseInfo(const TransceiverInfo& transceiverInfo) {
  const TcvrState& tcvrState = *can_throw(transceiverInfo.tcvrState());
  const TcvrStats& tcvrStats = *can_throw(transceiverInfo.tcvrStats());
  auto globalSensors = tcvrStats.sensor();

  if (globalSensors) {
    printGlobalInterruptFlags(*globalSensors);
  }
  printChannelInterruptFlags(*(tcvrStats.channels()));
  if (auto thresholds = tcvrState.thresholds()) {
    printThresholds(*thresholds);
  }
}

void printVendorInfo(const TransceiverInfo& transceiverInfo) {
  if (auto vendor = can_throw(transceiverInfo.tcvrState())->vendor()) {
    auto vendorInfo = *vendor;
    printf("  Vendor Info:\n");
    auto name = *(vendorInfo.name());
    auto pn = *(vendorInfo.partNumber());
    auto rev = *(vendorInfo.rev());
    auto sn = *(vendorInfo.serialNumber());
    auto dateCode = *(vendorInfo.dateCode());
    printf("    Vendor: %s\n", name.c_str());
    printf("    Vendor PN: %s\n", pn.c_str());
    printf("    Vendor Rev: %s\n", rev.c_str());
    printf("    Vendor SN: %s\n", sn.c_str());
    printf("    Date Code: %s\n", dateCode.c_str());
  }
}

void printCableInfo(const Cable& cable) {
  printf("  Cable Settings:\n");
  if (auto sm = cable.singleMode()) {
    printf("    Length (SMF): %d m\n", *sm);
  }
  if (auto om5 = cable.om5()) {
    printf("    Length (OM5): %d m\n", *om5);
  }
  if (auto om4 = cable.om4()) {
    printf("    Length (OM4): %d m\n", *om4);
  }
  if (auto om3 = cable.om3()) {
    printf("    Length (OM3): %d m\n", *om3);
  }
  if (auto om2 = cable.om2()) {
    printf("    Length (OM2): %d m\n", *om2);
  }
  if (auto om1 = cable.om1()) {
    printf("    Length (OM1): %d m\n", *om1);
  }
  if (auto copper = cable.copper()) {
    printf("    Length (Copper): %d m\n", *copper);
  }
  if (auto copperLength = cable.length()) {
    printf("    Length (Copper dM): %.1f m\n", *copperLength);
  }
  if (auto gauge = cable.gauge()) {
    printf("    DAC Cable Gauge: %d\n", *gauge);
  }
}

void printDomMonitors(const TransceiverInfo& transceiverInfo) {
  const TcvrStats& tcvrStats = *can_throw(transceiverInfo.tcvrStats());
  auto globalSensors = tcvrStats.sensor();
  printLaneDomMonitors(*(tcvrStats.channels()));
  if (globalSensors) {
    printGlobalDomMonitors(*globalSensors);
  }
}

void printSignalsAndSettings(const TransceiverInfo& transceiverInfo) {
  const TcvrState& tcvrState = *can_throw(transceiverInfo.tcvrState());
  auto settings = *(tcvrState.settings());
  if (auto hostSignals = tcvrState.hostLaneSignals()) {
    printHostLaneSignals(*hostSignals);
  }
  if (auto mediaSignals = tcvrState.mediaLaneSignals()) {
    printMediaLaneSignals(*mediaSignals);
  }
  if (auto hostSettings = settings.hostLaneSettings()) {
    printHostLaneSettings(*hostSettings);
  }
  if (auto mediaSettings = settings.mediaLaneSettings()) {
    printMediaLaneSettings(*mediaSettings);
  }
}

void printSff8472DetailService(
    const TransceiverInfo& transceiverInfo,
    unsigned int port,
    bool /* verbose */) {
  const TcvrState& tcvrState = *can_throw(transceiverInfo.tcvrState());
  auto settings = *(tcvrState.settings());

  // ------ Module Status -------
  printf("  Module Status:\n");
  if (auto identifier = tcvrState.identifier()) {
    printf(
        "    Transceiver Identifier: %s\n",
        apache::thrift::util::enumNameSafe(*identifier).c_str());
  }
  if (auto stateMachineState = tcvrState.stateMachineState()) {
    printf(
        "    StateMachine State: %s\n",
        apache::thrift::util::enumNameSafe(*stateMachineState).c_str());
  }

  printManagementInterface(
      transceiverInfo, "    Transceiver Management Interface: %s\n");
  printMediaInterfaceCode(
      can_throw(*tcvrState.moduleMediaInterface()),
      "    Module Media Interface: %s\n");
  if (auto mediaInterfaceId = settings.mediaInterface()) {
    printf(
        "  Current Media Interface: %s\n",
        apache::thrift::util::enumNameSafe(
            (*mediaInterfaceId)[0].media()->get_ethernet10GComplianceCode())
            .c_str());
  }
  printSignalsAndSettings(transceiverInfo);
  printDomMonitors(transceiverInfo);

  // if (verbose) {
  //   printVerboseInfo(transceiverInfo);
  // }

  printVendorInfo(transceiverInfo);

  if (auto timeCollected = *transceiverInfo.tcvrState()->timeCollected()) {
    printf("  Time collected: %s\n", getLocalTime(timeCollected).c_str());
  }
}

void printSffDetailService(
    const TransceiverInfo& transceiverInfo,
    unsigned int port,
    bool verbose) {
  const TcvrState& tcvrState = *can_throw(transceiverInfo.tcvrState());

  auto& settings = *(tcvrState.settings());

  // ------ Module Status -------
  printf("  Module Status:\n");
  if (auto identifier = tcvrState.identifier()) {
    printf(
        "    Transceiver Identifier: %s\n",
        apache::thrift::util::enumNameSafe(*identifier).c_str());
  }
  if (auto status = tcvrState.status()) {
    printf("    InterruptL: 0x%02x\n", *(status->interruptL()));
    printf("    Data_Not_Ready: 0x%02x\n", *(status->dataNotReady()));
  }
  printMediaInterfaceCode(
      can_throw(*tcvrState.moduleMediaInterface()),
      "    Module Media Interface: %s\n");
  if (auto ext = tcvrState.extendedSpecificationComplianceCode()) {
    printf(
        "    Extended Specification Compliance Code: %s\n",
        apache::thrift::util::enumNameSafe(*(ext)).c_str());
  }
  if (auto stateMachineState = tcvrState.stateMachineState()) {
    printf(
        "    StateMachine State: %s\n",
        apache::thrift::util::enumNameSafe(*stateMachineState).c_str());
  }

  printManagementInterface(
      transceiverInfo, "    Transceiver Management Interface: %s\n");
  printSignalsAndSettings(transceiverInfo);
  printDomMonitors(transceiverInfo);

  if (verbose) {
    printVerboseInfo(transceiverInfo);
  }

  printVendorInfo(transceiverInfo);

  if (auto cable = tcvrState.cable()) {
    printCableInfo(*cable);
  }
  printf(
      "  EEPROM Checksum: %s\n",
      *tcvrState.eepromCsumValid() ? "Valid" : "Invalid");
  printf("  Module Control:\n");
  printf(
      "    Rate Select: %s\n",
      apache::thrift::util::enumNameSafe(*(settings.rateSelect())).c_str());
  printf(
      "    Rate Select Setting: %s\n",
      apache::thrift::util::enumNameSafe(*(settings.rateSelectSetting()))
          .c_str());
  printf(
      "    CDR support:  TX: %s\tRX: %s\n",
      apache::thrift::util::enumNameSafe(*(settings.cdrTx())).c_str(),
      apache::thrift::util::enumNameSafe(*(settings.cdrRx())).c_str());
  printf(
      "    Power Measurement: %s\n",
      apache::thrift::util::enumNameSafe(*(settings.powerMeasurement()))
          .c_str());
  printf(
      "    Power Control: %s\n",
      apache::thrift::util::enumNameSafe(*(settings.powerControl())).c_str());
  if (auto timeCollected = *transceiverInfo.tcvrState()->timeCollected()) {
    printf("  Time collected: %s\n", getLocalTime(timeCollected).c_str());
  }
}

void printSffDetail(const DOMDataUnion& domDataUnion, unsigned int port) {
  Sff8636Data sffData = domDataUnion.get_sff8636();
  auto lowerBuf = sffData.lower()->data();
  auto page0Buf = sffData.page0()->data();

  printf("  ID: %#04x\n", lowerBuf[0]);
  printf("  Status: 0x%02x 0x%02x\n", lowerBuf[1], lowerBuf[2]);
  printf("  Module State: 0x%02x\n", lowerBuf[3]);
  printf(
      "  EEPROM Checksum: %s\n",
      getEepromCsumStatus(domDataUnion) ? "Valid" : "Invalid");

  printf("  Interrupt Flags:\n");
  printf("    LOS: 0x%02x\n", lowerBuf[3]);
  printf("    Fault: 0x%02x\n", lowerBuf[4]);
  printf("    LOL: 0x%02x\n", lowerBuf[5]);
  printf("    Temp: 0x%02x\n", lowerBuf[6]);
  printf("    Vcc: 0x%02x\n", lowerBuf[7]);
  printf("    Rx Power: 0x%02x 0x%02x\n", lowerBuf[9], lowerBuf[10]);
  printf("    Tx Power: 0x%02x 0x%02x\n", lowerBuf[13], lowerBuf[14]);
  printf("    Tx Bias: 0x%02x 0x%02x\n", lowerBuf[11], lowerBuf[12]);
  printf("    Reserved Set 4: 0x%02x 0x%02x\n", lowerBuf[15], lowerBuf[16]);
  printf("    Reserved Set 5: 0x%02x 0x%02x\n", lowerBuf[17], lowerBuf[18]);
  printf(
      "    Vendor Defined: 0x%02x 0x%02x 0x%02x\n",
      lowerBuf[19],
      lowerBuf[20],
      lowerBuf[21]);

  auto temp = static_cast<int8_t>(lowerBuf[22]) + (lowerBuf[23] / 256.0);
  printf("  Temperature: %f C\n", temp);
  uint16_t voltage = (lowerBuf[26] << 8) | lowerBuf[27];
  printf("  Supply Voltage: %f V\n", voltage / 10000.0);

  printf(
      "  Channel Data:  %12s    %12s    %12s    %12s\n",
      "RX Power",
      "TX Power",
      "TX Bias",
      "Rx SNR");
  printChannelMonitor(1, lowerBuf, 34, 35, 42, 43, 50, 51);
  printChannelMonitor(2, lowerBuf, 36, 37, 44, 45, 52, 53);
  printChannelMonitor(3, lowerBuf, 38, 39, 46, 47, 54, 55);
  printChannelMonitor(4, lowerBuf, 40, 41, 48, 49, 56, 57);
  printf(
      "    Power measurement is %s\n",
      (page0Buf[92] & 0x04) ? "supported" : "unsupported");
  printf(
      "    Reported RX Power is %s\n",
      (page0Buf[92] & 0x08) ? "average power" : "OMA");

  printf(
      "  Power set:  0x%02x\tExtended ID:  0x%02x\t"
      "Ethernet Compliance:  0x%02x\n",
      lowerBuf[93],
      page0Buf[1],
      page0Buf[3]);
  printf("  TX disable bits: 0x%02x\n", lowerBuf[86]);
  printf(
      "  Rate select is %s\n",
      (page0Buf[93] & 0x0c) ? "supported" : "unsupported");
  printf("  RX rate select bits: 0x%02x\n", lowerBuf[87]);
  printf("  TX rate select bits: 0x%02x\n", lowerBuf[88]);
  printf(
      "  CDR support:  TX: %s\tRX: %s\n",
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
  printf(
      "  Spec compliance: "
      "0x%02x 0x%02x 0x%02x 0x%02x"
      "0x%02x 0x%02x 0x%02x 0x%02x\n",
      page0Buf[3],
      page0Buf[4],
      page0Buf[5],
      page0Buf[6],
      page0Buf[7],
      page0Buf[8],
      page0Buf[9],
      page0Buf[10]);
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
  if (cableGauge > 0) {
    printf("  DAC Cable Gauge: %d\n", cableGauge);
  }
  printf("  Device Tech: 0x%02x\n", page0Buf[19]);
  printf("  Ext Module: 0x%02x\n", page0Buf[36]);
  printf("  Wavelength tolerance: 0x%02x 0x%02x\n", page0Buf[60], page0Buf[61]);
  printf("  Max case temp: %dC\n", page0Buf[62]);
  printf("  CC_BASE: 0x%02x\n", page0Buf[63]);
  printf(
      "  Options: 0x%02x 0x%02x 0x%02x 0x%02x\n",
      page0Buf[64],
      page0Buf[65],
      page0Buf[66],
      page0Buf[67]);
  printf("  DOM Type: 0x%02x\n", page0Buf[92]);
  printf("  Enhanced Options: 0x%02x\n", page0Buf[93]);
  printf("  Reserved: 0x%02x\n", page0Buf[94]);
  printf("  CC_EXT: 0x%02x\n", page0Buf[95]);
  printf("  Vendor Specific:\n");
  printf(
      "    %02x %02x %02x %02x %02x %02x %02x %02x"
      "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
      page0Buf[96],
      page0Buf[97],
      page0Buf[98],
      page0Buf[99],
      page0Buf[100],
      page0Buf[101],
      page0Buf[102],
      page0Buf[103],
      page0Buf[104],
      page0Buf[105],
      page0Buf[106],
      page0Buf[107],
      page0Buf[108],
      page0Buf[109],
      page0Buf[110],
      page0Buf[111]);
  printf(
      "    %02x %02x %02x %02x %02x %02x %02x %02x"
      "  %02x %02x %02x %02x %02x %02x %02x %02x\n",
      page0Buf[112],
      page0Buf[113],
      page0Buf[114],
      page0Buf[115],
      page0Buf[116],
      page0Buf[117],
      page0Buf[118],
      page0Buf[119],
      page0Buf[120],
      page0Buf[121],
      page0Buf[122],
      page0Buf[123],
      page0Buf[124],
      page0Buf[125],
      page0Buf[126],
      page0Buf[127]);

  printf("  Vendor: %s\n", vendor.str().c_str());
  printf(
      "  Vendor OUI: %02x:%02x:%02x\n",
      lowerBuf[165 - 128],
      lowerBuf[166 - 128],
      lowerBuf[167 - 128]);
  printf("  Vendor PN: %s\n", vendorPN.str().c_str());
  printf("  Vendor Rev: %s\n", vendorRev.str().c_str());
  printf("  Vendor SN: %s\n", vendorSN.str().c_str());
  printf("  Date Code: %s\n", vendorDate.str().c_str());

  // print page3 values
  if (!sffData.page3()) {
    return;
  }

  auto page3Buf = sffData.page3().value_unchecked().data();

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

  if (auto timeCollected = sffData.timeCollected()) {
    printf("  Time collected: %s\n", getLocalTime(*timeCollected).c_str());
  }
}

void printCmisDetailService(
    const TransceiverInfo& transceiverInfo,
    unsigned int port,
    bool verbose) {
  const TcvrState& tcvrState = *can_throw(transceiverInfo.tcvrState());
  const TcvrStats& tcvrStats = *can_throw(transceiverInfo.tcvrStats());

  auto moduleStatus = tcvrState.status();
  auto channels = *(tcvrStats.channels());
  auto settings = *(tcvrState.settings());

  printManagementInterface(
      transceiverInfo, "  Transceiver Management Interface: %s\n");

  if (moduleStatus && moduleStatus->cmisModuleState()) {
    printf(
        "  Module State: %s\n",
        apache::thrift::util::enumNameSafe(*(moduleStatus->cmisModuleState()))
            .c_str());
  }
  if (auto stateMachineState = tcvrState.stateMachineState()) {
    printf(
        "    StateMachine State: %s\n",
        apache::thrift::util::enumNameSafe(*stateMachineState).c_str());
  }
  printMediaInterfaceCode(
      can_throw(*tcvrState.moduleMediaInterface()),
      "  Module Media Interface: %s\n");
  if (auto mediaInterfaceId = settings.mediaInterface()) {
    std::string mediaInterface;
    if ((*mediaInterfaceId)[0].media()->getType() ==
        MediaInterfaceUnion::Type::smfCode) {
      mediaInterface = apache::thrift::util::enumNameSafe(
          (*mediaInterfaceId)[0].media()->get_smfCode());
    } else if (
        (*mediaInterfaceId)[0].media()->getType() ==
        MediaInterfaceUnion::Type::passiveCuCode) {
      mediaInterface = apache::thrift::util::enumNameSafe(
          (*mediaInterfaceId)[0].media()->get_passiveCuCode());
    }
    printf("  Current Media Interface: %s\n", mediaInterface.c_str());
  }
  printf(
      "  Power Control: %s\n",
      apache::thrift::util::enumNameSafe(*settings.powerControl()).c_str());

  if (moduleStatus && moduleStatus->fwStatus()) {
    auto fwStatus = *(moduleStatus->fwStatus());
    if (auto version = fwStatus.version()) {
      uint8_t integerPart, fractionalPart = 0;
      size_t pos = version->find('.');
      if (pos != std::string::npos) {
        integerPart = stoi(version->substr(0, pos));
        fractionalPart = stoi(version->substr(pos + 1));
      } else {
        integerPart = stoi(version->substr(0));
      }
      printf("  FW Version: %x.%x\n", integerPart, fractionalPart);
    }
    if (auto fwFault = fwStatus.fwFault()) {
      printf("  Firmware fault: 0x%x\n", *fwFault);
    }
  }

  if (verbose) {
    printVerboseInfo(transceiverInfo);
  }
  printf(
      "  EEPROM Checksum: %s\n",
      *tcvrState.eepromCsumValid() ? "Valid" : "Invalid");
  printSignalsAndSettings(transceiverInfo);
  printDomMonitors(transceiverInfo);
  printVendorInfo(transceiverInfo);
  if (auto timeCollected = *transceiverInfo.tcvrState()->timeCollected()) {
    printf("  Time collected: %s\n", getLocalTime(timeCollected).c_str());
  }
}

void printCmisDetail(const DOMDataUnion& domDataUnion, unsigned int port) {
  int i = 0; // For the index of lane
  CmisData cmisData = domDataUnion.get_cmis();
  const uint8_t *lowerBuf, *page0Buf, *page01Buf, *page10Buf, *page11Buf,
      *page14Buf;
  lowerBuf = cmisData.lower()->data();
  page0Buf = cmisData.page0()->data();

  // Some CMIS optics like Blanco bypass modules have flat memory so we need
  // to exclude other pages for these modules
  bool flatMem = ((lowerBuf[2] & 0x80) != 0);

  if (!flatMem) {
    page01Buf = can_throw(cmisData.page01())->data();
    page10Buf = can_throw(cmisData.page10())->data();
    page11Buf = can_throw(cmisData.page11())->data();
    page14Buf = can_throw(cmisData.page14())->data();
  }

  printf("  Module Interface Type: CMIS (200G or above)\n");

  printf(
      "  Module State: %s\n",
      getStateNameString(lowerBuf[3] >> 1, kCmisModuleStateMapping).c_str());

  if (!flatMem) {
    auto ApSel = page11Buf[78] >> 4;
    auto ApCode = lowerBuf[86 + (ApSel - 1) * 4 + 1];
    printf(
        "  Application Selected: %s\n",
        getStateNameString(ApCode, kCmisAppNameMapping).c_str());
  }
  printf("  Low power: 0x%x\n", (lowerBuf[26] >> 6) & 0x1);
  printf("  Low power forced: 0x%x\n", (lowerBuf[26] >> 4) & 0x1);

  printf("  Module FW Version: %x.%x\n", lowerBuf[39], lowerBuf[40]);
  if (!flatMem) {
    printf("  DSP FW Version: %x.%x\n", page01Buf[66], page01Buf[67]);
    printf("  Build Rev: %x.%x\n", page01Buf[68], page01Buf[69]);
  }
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
  printf(
      "  EEPROM Checksum: %s\n",
      getEepromCsumStatus(domDataUnion) ? "Valid" : "Invalid");

  auto temp = static_cast<int8_t>(lowerBuf[14]) + (lowerBuf[15] / 256.0);
  printf("  Temperature: %f C\n", temp);

  printf(
      "  VCC: %f V\n", CmisFieldInfo::getVcc(lowerBuf[16] << 8 | lowerBuf[17]));

  if (!flatMem) {
    printf("\nPer Lane status: \n");
    printf(
        "Lanes             1        2        3        4        5        6        7        8\n");
    printf("Datapath de-init  ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page10Buf[0] >> i) & 1);
    }
    printf("\nTx disable        ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page10Buf[2] >> i) & 1);
    }
    printf("\nTx squelch bmap   ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page10Buf[4] >> i) & 1);
    }
    printf("\nRx Out disable    ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page10Buf[10] >> i) & 1);
    }
    printf("\nRx Sqlch disable  ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page10Buf[11] >> i) & 1);
    }
    printf("\nHost lane state   ");
    for (i = 0; i < 4; i++) {
      printf(
          "%-7s  %-7s  ",
          getStateNameString(page11Buf[i] & 0xf, kCmisLaneStateMapping).c_str(),
          getStateNameString((page11Buf[i] >> 4) & 0xf, kCmisLaneStateMapping)
              .c_str());
    }
    printf("\nTx fault          ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[7] >> i) & 1);
    }
    printf("\nTx LOS            ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[8] >> i) & 1);
    }
    printf("\nTx LOL            ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[9] >> i) & 1);
    }
    printf("\nTx PWR alarm Hi   ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[11] >> i) & 1);
    }
    printf("\nTx PWR alarm Lo   ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[12] >> i) & 1);
    }
    printf("\nTx PWR warn Hi    ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[13] >> i) & 1);
    }
    printf("\nTx PWR warn Lo    ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[14] >> i) & 1);
    }
    printf("\nRx LOS            ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[19] >> i) & 1);
    }
    printf("\nRx LOL            ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[20] >> i) & 1);
    }
    printf("\nRx PWR alarm Hi   ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[21] >> i) & 1);
    }
    printf("\nRx PWR alarm Lo   ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[22] >> i) & 1);
    }
    printf("\nRx PWR warn Hi    ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[23] >> i) & 1);
    }
    printf("\nRx PWR warn Lo    ");
    for (i = 0; i < 8; i++) {
      printf("%d        ", (page11Buf[24] >> i) & 1);
    }
    printf("\nTX Power (mW)     ");
    for (i = 0; i < 8; i++) {
      printf(
          "%.3f    ",
          ((page11Buf[26 + i * 2] << 8 | page11Buf[27 + i * 2])) * 0.0001);
    }
    printf("\nRX Power (mW)     ");
    for (i = 0; i < 8; i++) {
      printf(
          "%.3f    ",
          ((page11Buf[58 + i * 2] << 8 | page11Buf[59 + i * 2])) * 0.0001);
    }
    printf("\nRx SNR            ");
    for (i = 0; i < 8; i++) {
      printf(
          "%05.04g    ",
          (CmisFieldInfo::getSnr(
              page14Buf[113 + i * 2] << 8 | page14Buf[112 + i * 2])));
    }
    printf("\nRX PreCur (dB)    ");
    for (i = 0; i < 8; i++) {
      printf(
          "%.1f      ",
          CmisFieldInfo::getPreCursor(
              (page11Buf[95 + i / 2] >> ((i % 2) * 4)) & 0x0f));
    }
    printf("\nRX PostCur (dB)   ");
    for (i = 0; i < 8; i++) {
      printf(
          "%.1f      ",
          CmisFieldInfo::getPostCursor(
              (page11Buf[99 + i / 2] >> ((i % 2) * 4)) & 0x0f));
    }
    printf("\nRX Ampl (mV)      ");
    for (i = 0; i < 8; i++) {
      auto ampRange = CmisFieldInfo::getAmplitude(
          (page11Buf[103 + i / 2] >> ((i % 2) * 4)) & 0x0f);
      char rangeStr[16];
      sprintf(rangeStr, "%d-%d      ", ampRange.first, ampRange.second);
      rangeStr[9] = 0;
      printf("%s", rangeStr);
    }
  }
  if (auto timeCollected = cmisData.timeCollected()) {
    printf("\nTime collected: %s", getLocalTime(*timeCollected).c_str());
  }
  printf("\n\n");
}

void printPortDetail(
    const DOMDataUnion& domDataUnion,
    unsigned int port,
    const std::string& portNames) {
  if (domDataUnion.__EMPTY__) {
    fprintf(stderr, "DOMDataUnion object is empty\n");
    return;
  }
  printf("Port %d\n", port);
  printf("Logical Ports: %s\n", portNames.c_str());
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
    bool verbose,
    const std::string& portNames) {
  if (auto mgmtInterface = can_throw(transceiverInfo.tcvrState())
                               ->transceiverManagementInterface()) {
    printf("Port %d\n", port);
    printf("Logical Ports: %s\n", portNames.c_str());
    if (*mgmtInterface == TransceiverManagementInterface::SFF) {
      printSffDetailService(transceiverInfo, port, verbose);
    } else if (*mgmtInterface == TransceiverManagementInterface::SFF8472) {
      printSff8472DetailService(transceiverInfo, port, verbose);
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
    } catch (const std::exception&) {
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
 * platform the called function creates Platform specific TransceiverApi
 * object and returns it. For I2c controlled platform the called function
 * creates TransceiverPlatformI2cApi object and keeps the platform specific
 * I2CBus object raw pointer inside it. The returned object's Qsfp control
 * function is called here to use appropriate Fpga/I2c Api in this function.
 */
bool doQsfpHardReset(TransceiverI2CApi* bus, unsigned int port) {
  // Call the function to get TransceiverPlatformApi object. For Fpga
  // controlled platform it returns Platform specific TransceiverApi object.
  // For I2c controlled platform it returns TransceiverPlatformI2cApi
  // which contains "bus" as it's internal variable
  auto busAndError = getTransceiverPlatformAPI(bus);

  if (busAndError.second) {
    fprintf(
        stderr,
        "Trying to doQsfpHardReset, Couldn't getTransceiverPlatformAPI, error out.\n");
    return false;
  }

  auto qsfpBus = std::move(busAndError.first);

  // This will eventuall call the Fpga or the I2C based platform specific
  // Qsfp reset function
  qsfpBus->triggerQsfpHardReset(port);

  return true;
}

int resetQsfp(const std::vector<std::string>& ports, folly::EventBase& evb) {
  if (ports.empty()) {
    XLOG(ERR) << "Please specify QSFP ports to be reset";
    return EX_USAGE;
  }

  ResetType resetType;
  apache::thrift::TEnumTraits<ResetType>::findValue(
      FLAGS_qsfp_reset_type, &resetType);

  switch (static_cast<ResetType>(resetType)) {
    case ResetType::HARD_RESET:
      break;
    case ResetType::INVALID:
    default:
      XLOG(ERR) << fmt::format("Invalid reset type: {}", FLAGS_qsfp_reset_type);
      return EX_USAGE;
  };

  ResetAction resetAction;
  apache::thrift::TEnumTraits<ResetAction>::findValue(
      FLAGS_qsfp_reset_action, &resetAction);

  switch (static_cast<ResetAction>(resetAction)) {
    case ResetAction::RESET_THEN_CLEAR:
    case ResetAction::RESET:
    case ResetAction::CLEAR_RESET:
      break;
    case ResetAction::INVALID:
    default:
      XLOG(ERR) << fmt::format(
          "Invalid reset action: {}", FLAGS_qsfp_reset_action);
      return EX_USAGE;
  };

  try {
    auto client = getQsfpClient(evb);
    client->sync_resetTransceiver(ports, resetType, resetAction);
    XLOG(INFO) << fmt::format(
        "Successfully reset ports via qsfp_service with resetType {} resetAction {}",
        FLAGS_qsfp_reset_type,
        FLAGS_qsfp_reset_action);
  } catch (const std::exception& ex) {
    XLOG(ERR) << fmt::format("Error reseting ports: {:s}", ex.what());
    return EX_SOFTWARE;
  }

  return EX_OK;
}

int dumpTransceiverI2cLog(
    const std::vector<std::string>& ports,
    folly::EventBase& evb) {
  auto ret = EX_OK;
  std::vector<std::string> successPorts;
  std::vector<std::string> failedPorts;
  for (auto& port : ports) {
    try {
      auto client = getQsfpClient(evb);
      client->sync_dumpTransceiverI2cLog(port);
      successPorts.push_back(port);
    } catch (const std::exception&) {
      failedPorts.push_back(port);
      ret = EX_SOFTWARE;
    }
  }

  if (!successPorts.empty()) {
    XLOG(INFO) << fmt::format(
        "Successfully dumped i2c log for ports: {:s}",
        folly::join(",", successPorts));
  }
  if (!failedPorts.empty()) {
    XLOG(INFO) << fmt::format(
        "Failed to dump i2c log for ports: {:s}",
        folly::join(",", failedPorts));
  }

  return ret;
}

bool setTransceiverLoopback(
    DirectI2cInfo i2cInfo,
    std::vector<std::string> portList,
    LoopbackMode mode) {
  TransceiverManagementInterface managementInterface;

  if (FLAGS_direct_i2c) {
    bool result = true;
    TransceiverI2CApi* bus = i2cInfo.bus;
    TransceiverManager* wedgeManager = i2cInfo.transceiverManager;

    for (auto& portName : portList) {
      auto port = wedgeManager->getPortNameToModuleMap().at(portName) + 1;
      managementInterface = getModuleTypeDirect(bus, port);
      if (managementInterface != TransceiverManagementInterface::CMIS) {
        result = result && doMiniphotonLoopbackDirect(bus, port, mode);
      } else {
        if (mode == electricalLoopback || mode == noLoopback) {
          cmisHostInputLoopbackDirect(bus, port, mode);
        }
        if (mode == opticalLoopback || mode == noLoopback) {
          cmisMediaInputLoopbackDirect(bus, port, mode);
        }
      }
      printf(
          "QSFP port %s loopback mode setting to %s done\n",
          portName.c_str(),
          ((mode == electricalLoopback)    ? "electrical"
               : (mode == opticalLoopback) ? "opticalLoopback"
                                           : "noLoopback"));
    }
    return result;
  } else {
    if (!QsfpServiceDetector::getInstance()->isQsfpServiceActive()) {
      throw FbossError("qsfp_service not found running");
    }

    folly::EventBase& evb = QsfpUtilContainer::getInstance()->getEventBase();
    auto client = getQsfpClient(evb);

    for (auto& portName : portList) {
      try {
        if (mode == electricalLoopback) {
          client->sync_setPortLoopbackState(
              portName, phy::PortComponent::TRANSCEIVER_SYSTEM, true);
        } else if (mode == opticalLoopback) {
          client->sync_setPortLoopbackState(
              portName, phy::PortComponent::TRANSCEIVER_LINE, true);
        } else {
          client->sync_setPortLoopbackState(
              portName, phy::PortComponent::TRANSCEIVER_SYSTEM, false);
          client->sync_setPortLoopbackState(
              portName, phy::PortComponent::TRANSCEIVER_LINE, false);
        }

        printf("QSFP port %s loopback mode setting done\n", portName.c_str());
      } catch (const std::exception& ex) {
        fprintf(
            stderr,
            "Error setting loopback mode via qsfp_service: %s\n",
            ex.what());
        return false;
      }
    }
    return true;
  }
  return true;
}

bool doMiniphotonLoopbackDirect(
    TransceiverI2CApi* bus,
    unsigned int port,
    LoopbackMode mode) {
  try {
    // Make sure we have page128 selected.
    uint8_t page128 = 128;
    bus->moduleWrite(
        port, {TransceiverAccessParameter::ADDR_QSFP, 127, 1}, &page128);

    uint8_t loopback = 0;
    if (mode == electricalLoopback) {
      loopback = 0b01010101;
    } else if (mode == opticalLoopback) {
      loopback = 0b10101010;
    }
    fprintf(stderr, "loopback value: %x\n", loopback);
    bus->moduleWrite(
        port, {TransceiverAccessParameter::ADDR_QSFP, 245, 1}, &loopback);
  } catch (const I2cError&) {
    fprintf(stderr, "QSFP %d: fail to set loopback\n", port);
    return false;
  }
  return true;
}

void cmisHostInputLoopbackDirect(
    TransceiverI2CApi* bus,
    unsigned int port,
    LoopbackMode mode) {
  try {
    // Make sure we have page 0x13 selected.
    uint8_t page = 0x13;
    bus->moduleWrite(
        port,
        {TransceiverAccessParameter::ADDR_QSFP, 127, sizeof(page)},
        &page);

    uint8_t data;
    if (!FLAGS_skip_check) {
      bus->moduleRead(
          port,
          {TransceiverAccessParameter::ADDR_QSFP, 128, sizeof(data)},
          &data);
      if (!(data & 0x08)) {
        fprintf(
            stderr,
            "QSFP %d: Host side input loopback not supported, you may try --skip_check\n",
            port);
        return;
      }
    }

    data = (mode == electricalLoopback) ? 0xff : 0;
    bus->moduleWrite(
        port,
        {TransceiverAccessParameter::ADDR_QSFP, 183, sizeof(data)},
        &data);
  } catch (const I2cError&) {
    fprintf(stderr, "QSFP %d: fail to set loopback\n", port);
  }
}

void cmisMediaInputLoopbackDirect(
    TransceiverI2CApi* bus,
    unsigned int port,
    LoopbackMode mode) {
  try {
    // Make sure we have page 0x13 selected.
    uint8_t page = 0x13;
    bus->moduleWrite(
        port,
        {TransceiverAccessParameter::ADDR_QSFP, 127, sizeof(page)},
        &page);

    uint8_t data;
    if (!FLAGS_skip_check) {
      bus->moduleRead(
          port,
          {TransceiverAccessParameter::ADDR_QSFP, 128, sizeof(data)},
          &data);
      if (!(data & 0x02)) {
        fprintf(
            stderr,
            "QSFP %d: Media side input loopback not supported, you may try --skip_check\n",
            port);
        return;
      }
    }

    data = (mode == opticalLoopback) ? 0xff : 0;
    bus->moduleWrite(
        port,
        {TransceiverAccessParameter::ADDR_QSFP, 181, sizeof(data)},
        &data);
  } catch (const I2cError&) {
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
std::vector<int> getPidForProcess(std::string proccessName) {
  std::vector<int> pidList;

  // Look into the /proc directory
  DIR* dp = opendir("/proc");
  if (dp == nullptr) {
    return pidList;
  }

  // Look for all /proc/<pid>/cmdline to check if anything matches with
  // input processName
  struct dirent* dirEntry;
  while ((dirEntry = readdir(dp))) {
    int currPid = atoi(dirEntry->d_name);
    if (currPid <= 0) {
      // skip non user processes
      continue;
    }

    // This is proper user process, read cmdline
    std::string commandPath =
        std::string("/proc/") + dirEntry->d_name + "/cmdline";
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
 * thread. This directly invokes the bus->moduleRead/Write API to do the
 * module CDB based firnware upgrade
 */
bool cliModulefirmwareUpgrade(
    DirectI2cInfo i2cInfo,
    unsigned int port,
    std::string firmwareFilename) {
  // If the qsfp_service is running then this firmware upgrade command is most
  // likely to fail. Print warning and returning from here
  auto pidList = getPidForProcess("qsfp_service");
  if (!pidList.empty()) {
    printf("The qsfp_service seems to be running.\n");
    printf("The f/w upgrade CLI may not work reliably\n");
    printf(
        "Consider stopping qsfp_service and re-issue this upgrade command\n");
    return false;
  }

  auto domData = fetchDataFromLocalI2CBus(i2cInfo, port);
  CmisData cmisData = domData.get_cmis();
  auto dataUpper = cmisData.page0()->data();

  // Confirm module type is CMIS
  auto moduleType = getModuleTypeDirect(i2cInfo.bus, port);
  if (moduleType != TransceiverManagementInterface::CMIS) {
    // If device can't be determined as cmis then check page 0 register 128
    // also to identify its type as in some case corrupted image wipes out all
    // lower page registers
    if (dataUpper[0] ==
            static_cast<uint8_t>(TransceiverModuleIdentifier::QSFP_PLUS_CMIS) ||
        dataUpper[0] ==
            static_cast<uint8_t>(TransceiverModuleIdentifier::QSFP_DD)) {
      printf(
          "Optics current firmware seems to  be corrupted, proceeding with f/w upgrade\n");
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
    for (int i = 0; i < 16; i++) {
      modPartNo[i] = dataUpper[20 + i];
    }

    for (int i = 0; i < kNumModuleInfo; i++) {
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
  firmwareAttr.properties["msa_password"] =
      folly::to<std::string>(FLAGS_msa_password);
  firmwareAttr.properties["header_length"] =
      folly::to<std::string>(imageHdrLen);
  firmwareAttr.properties["image_type"] =
      FLAGS_dsp_image ? "dsp" : "application";
  auto fbossFwObj = std::make_unique<FbossFirmware>(firmwareAttr);
  auto qsfpImpl = std::make_unique<WedgeQsfp>(
      port - 1, i2cInfo.bus, i2cInfo.transceiverManager, /*logBuffer*/ nullptr);
  auto fwUpgradeObj = std::make_unique<CmisFirmwareUpgrader>(
      qsfpImpl.get(), port, fbossFwObj.get());

  // Do the standalone upgrade in the same process as wedge_qsfp_util
  bool ret = fwUpgradeObj->cmisModuleFirmwareUpgrade();

  if (ret) {
    printf(
        "Firmware download successful, the module is running desired firmware\n");
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
 * the upgrade needs to be performed. It calls the function bucketize to
 * create separate buckets of optics for upgrade. Then the threads are created
 * for each bucket and each thread takes care of upgrading the optics in that
 * bucket. The idea here is that one thread will take care of upgrading the
 * optics belonging to one controller so that two ports of the same controller
 * can't upgrade at the same time whereas multiple ports belonging to
 * different controller can upgrade at the same time. This function waits for
 * all threads to finish.
 */
bool cliModulefirmwareUpgrade(
    DirectI2cInfo i2cInfo,
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
  std::vector<unsigned int> finalModlist =
      getUpgradeModList(i2cInfo, modlist, FLAGS_module_type, FLAGS_fw_version);

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
    auto domData = fetchDataFromLocalI2CBus(i2cInfo, bucket[0][0]);
    CmisData cmisData = domData.get_cmis();
    auto dataUpper = cmisData.page0()->data();

    std::array<uint8_t, 16> modPartNo;
    for (int i = 0; i < 16; i++) {
      modPartNo[i] = dataUpper[20 + i];
    }

    for (int i = 0; i < kNumModuleInfo; i++) {
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
        fwUpgradeThreadHandler,
        i2cInfo,
        bucketrow,
        firmwareFilename,
        imageHdrLen);
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
    DirectI2cInfo i2cInfo,
    std::vector<unsigned int> modlist,
    std::string firmwareFilename,
    uint32_t imageHdrLen) {
  std::array<uint8_t, 2> versionNumber;

  for (auto module : modlist) {
    // Create FbossFirmware object using firmware filename and msa password,
    // header length as properties
    FbossFirmware::FwAttributes firmwareAttr;
    firmwareAttr.filename = firmwareFilename;
    firmwareAttr.properties["msa_password"] =
        folly::to<std::string>(FLAGS_msa_password);
    firmwareAttr.properties["header_length"] =
        folly::to<std::string>(imageHdrLen);
    firmwareAttr.properties["image_type"] =
        FLAGS_dsp_image ? "dsp" : "application";
    auto fbossFwObj = std::make_unique<FbossFirmware>(firmwareAttr);
    auto qsfpImpl = std::make_unique<WedgeQsfp>(
        module - 1,
        i2cInfo.bus,
        i2cInfo.transceiverManager,
        /*logBuffer*/ nullptr);
    auto fwUpgradeObj = std::make_unique<CmisFirmwareUpgrader>(
        qsfpImpl.get(), module, fbossFwObj.get());

    // Do the upgrade in this thread
    bool ret = fwUpgradeObj->cmisModuleFirmwareUpgrade();

    if (ret) {
      printf(
          "Firmware download successful for module %d, the module is running desired firmware\n",
          module);
    } else {
      printf(
          "Firmware upgrade failed for module %d, you may retry the same command\n",
          module);
    }

    // Find out the current version running on module
    i2cInfo.bus->moduleRead(
        module,
        {TransceiverAccessParameter::ADDR_QSFP, 39, 2},
        versionNumber.data());
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
 * This function returns the list of such eligible modules for firmware
 * upgrade
 */
std::vector<unsigned int> getUpgradeModList(
    DirectI2cInfo i2cInfo,
    std::vector<unsigned int> portlist,
    std::string moduleType,
    std::string fwVer) {
  std::vector<std::string> partNoList;
  std::vector<unsigned int> modlist;

  // Check if we have the mapping for this user specified module type to part
  // number
  if (CmisFirmwareUpgrader::partNoMap.find(moduleType) ==
      CmisFirmwareUpgrader::partNoMap.end()) {
    printf("Module Type is invalid\n");
    return modlist;
  }

  partNoList = CmisFirmwareUpgrader::partNoMap[moduleType];

  for (unsigned int module : portlist) {
    std::array<uint8_t, 16> partNo;
    std::array<uint8_t, 2> fwVerNo;
    std::array<char, 16> baseFwVerStr;
    std::string tempPartNo, tempFwVerStr;

    // Check if the module is present
    if (!i2cInfo.bus->isPresent(module)) {
      continue;
    }

    auto qsfpImpl = std::make_unique<WedgeQsfp>(
        module - 1,
        i2cInfo.bus,
        i2cInfo.transceiverManager,
        /*logBuffer*/ nullptr);
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
    if (std::find(partNoList.begin(), partNoList.end(), tempPartNo) ==
        partNoList.end()) {
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
 * one i2c controller for 8 modules, if the input  is the list of these
 * modules: 1 3 5 8 12 15 21 32 38 46 55 59 60 83 88 122 124 128 Then these
 * will be divided in following 9 buckets: Bucket: 1 3 5 8 Bucket: 12 15
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
std::vector<unsigned int> portRangeStrToPortList(std::string portQualifier) {
  int lastPos = 0, nextPos;
  int beg, end;
  bool emptyStr;
  std::string tempString;
  std::vector<std::string> portRangeStrings;
  std::vector<unsigned int> portRangeList;

  // Create a list of substrings separated by command ","
  // These substrings will contain single port like "5" or port range like
  // "7-9"
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
void get_module_fw_info(
    DirectI2cInfo i2cInfo,
    unsigned int moduleA,
    unsigned int moduleB) {
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

    if (!i2cInfo.bus->isPresent(module)) {
      continue;
    }

    auto moduleType = getModuleTypeDirect(i2cInfo.bus, module);
    if (moduleType != TransceiverManagementInterface::CMIS) {
      continue;
    }

    DOMDataUnion tempDomData = fetchDataFromLocalI2CBus(i2cInfo, module);
    CmisData cmisData = tempDomData.get_cmis();
    auto dataLower = cmisData.lower()->data();
    auto dataUpper = cmisData.page0()->data();

    fwVer[0] = dataLower[39];
    fwVer[1] = dataLower[40];

    memcpy(&vendor[0], &dataUpper[1], 16);
    memcpy(&partNo[0], &dataUpper[20], 16);

    printf("%2d         ", module);
    for (int i = 0; i < 16; i++) {
      printf("%c", vendor[i]);
    }

    printf("     ");
    for (int i = 0; i < 16; i++) {
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
 * [2] wedge_qsfp_util <module-id> --cdb_command --command_code <command-code>
 *     --cdb_payload <cdb-payload>
 *     Here the cdb-payload is a string containing payload bytes, ie: "100
 * 200"
 */

void doCdbCommand(DirectI2cInfo i2cInfo, unsigned int module) {
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
  auto qsfpImpl = std::make_unique<WedgeQsfp>(
      module - 1,
      i2cInfo.bus,
      i2cInfo.transceiverManager,
      /*logBuffer*/ nullptr);
  cdbBlock.selectCdbPage(qsfpImpl.get());
  cdbBlock.setMsaPassword(qsfpImpl.get(), FLAGS_msa_password);

  // Run the CDB command and get the output
  bool rc = cdbBlock.cmisRunCdbCommand(qsfpImpl.get());
  printf("CDB command %s\n", rc ? "Successful" : "Failed");

  uint8_t* pResp;
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
 * printVdmInfoViaService
 *
 * Get the VDM Performance Monitoring stats from qsfp_service ad display the
 * values
 */
bool printVdmInfoViaService(unsigned int port) {
  printf("Displaying VDM info for module %d via service:", port);

  folly::EventBase& evb = QsfpUtilContainer::getInstance()->getEventBase();
  std::vector<int32_t> portList;
  unsigned int zeroBasedPortId = port - 1;
  portList.push_back(zeroBasedPortId);
  auto tcvrInfo = fetchInfoFromQsfpService(portList, evb);
  if (tcvrInfo.find(zeroBasedPortId) == tcvrInfo.end()) {
    printf("Could not get TransceiverInfo for port %d\n", port);
    return false;
  }

  auto vdmStatsOpt =
      tcvrInfo[zeroBasedPortId].tcvrStats().value().vdmPerfMonitorStats();
  if (!vdmStatsOpt.has_value()) {
    printf("VDM stats not available for port %d\n", port);
    return false;
  }

  for (auto& [portName, mediaPortStats] :
       vdmStatsOpt.value().mediaPortVdmStats().value()) {
    printf("\n\nPort %s Media stats:\n", portName.c_str());
    printf("            Min        Max        Avg        Cur\n");
    printf(
        "  BER     : %.2e   %.2e   %.2e   %.2e\n",
        mediaPortStats.datapathBER().value().min().value(),
        mediaPortStats.datapathBER().value().max().value(),
        mediaPortStats.datapathBER().value().avg().value(),
        mediaPortStats.datapathBER().value().cur().value());
    printf(
        "  FEC Err : %.2e   %.2e   %.2e   %.2e\n",
        mediaPortStats.datapathErroredFrames().value().min().value(),
        mediaPortStats.datapathErroredFrames().value().max().value(),
        mediaPortStats.datapathErroredFrames().value().avg().value(),
        mediaPortStats.datapathErroredFrames().value().cur().value());
    if (mediaPortStats.fecTailCurr().has_value() &&
        mediaPortStats.fecTailMax().has_value()) {
      printf(
          "  FEC Tail: %-8s   %-8d   %-8s   %-8d\n",
          "N/A",
          mediaPortStats.fecTailMax().value(),
          "N/A",
          mediaPortStats.fecTailCurr().value());
    }

    if (!mediaPortStats.laneSNR().value().empty()) {
      printf("\n  Channels ->    ");
      for (auto& [channel, val] : mediaPortStats.laneSNR().value()) {
        printf("%d       ", channel);
      }
    }
    if (!mediaPortStats.laneSNR().value().empty()) {
      printf("\n  SNR        :  ");
      for (auto& [channel, val] : mediaPortStats.laneSNR().value()) {
        printf("%4.2f   ", val);
      }
    }
    if (!mediaPortStats.lanePam4Level0SD().value().empty()) {
      printf("\n  PAM4 L0 SD :  ");
      for (auto& [channel, val] : mediaPortStats.lanePam4Level0SD().value()) {
        printf("%4.2f    ", val);
      }
    }
    if (!mediaPortStats.lanePam4Level1SD().value().empty()) {
      printf("\n  PAM4 L1 SD :  ");
      for (auto& [channel, val] : mediaPortStats.lanePam4Level1SD().value()) {
        printf("%4.2f    ", val);
      }
    }
    if (!mediaPortStats.lanePam4Level2SD().value().empty()) {
      printf("\n  PAM4 L2 SD :  ");
      for (auto& [channel, val] : mediaPortStats.lanePam4Level2SD().value()) {
        printf("%4.2f    ", val);
      }
    }
    if (!mediaPortStats.lanePam4Level3SD().value().empty()) {
      printf("\n  PAM4 L3 SD :  ");
      for (auto& [channel, val] : mediaPortStats.lanePam4Level3SD().value()) {
        printf("%4.2f    ", val);
      }
    }
    if (!mediaPortStats.lanePam4MPI().value().empty()) {
      printf("\n  PAM4 MPI   :  ");
      for (auto& [channel, val] : mediaPortStats.lanePam4MPI().value()) {
        printf("%4.2f    ", val);
      }
    }
    if (!mediaPortStats.lanePam4LTP().value().empty()) {
      printf("\n  PAM4 LTP   :  ");
      for (auto& [channel, val] : mediaPortStats.lanePam4LTP().value()) {
        printf("%4.2f  ", val);
      }
    }
  }
  printf("\n");
  for (auto& [portName, hostPortStats] :
       vdmStatsOpt.value().hostPortVdmStats().value()) {
    printf("\nPort %s Host stats:\n", portName.c_str());
    printf("            Min        Max        Avg        Cur\n");
    printf(
        "  BER     : %.2e   %.2e   %.2e   %.2e\n",
        hostPortStats.datapathBER().value().min().value(),
        hostPortStats.datapathBER().value().max().value(),
        hostPortStats.datapathBER().value().avg().value(),
        hostPortStats.datapathBER().value().cur().value());
    printf(
        "  FEC Err : %.2e   %.2e   %.2e   %.2e\n",
        hostPortStats.datapathErroredFrames().value().min().value(),
        hostPortStats.datapathErroredFrames().value().max().value(),
        hostPortStats.datapathErroredFrames().value().avg().value(),
        hostPortStats.datapathErroredFrames().value().cur().value());
    if (hostPortStats.fecTailCurr().has_value() &&
        hostPortStats.fecTailMax().has_value()) {
      printf(
          "  FEC Tail: %-8s   %-8d   %-8s   %-8d\n",
          "N/A",
          hostPortStats.fecTailMax().value(),
          "N/A",
          hostPortStats.fecTailCurr().value());
    }

    if (!hostPortStats.laneSNR().value().empty()) {
      printf("\n  Channel ->    ");
      for (auto& [channel, val] : hostPortStats.laneSNR().value()) {
        printf("%d       ", channel);
      }
    }
    if (!hostPortStats.laneSNR().value().empty()) {
      printf("\n  SNR     :  ");
      for (auto& [channel, val] : hostPortStats.laneSNR().value()) {
        printf("%4.2f   ", val);
      }
    }
    if (!hostPortStats.lanePam4Level0SD().value().empty()) {
      printf("\n  PAM4 L0 SD :  ");
      for (auto& [channel, val] : hostPortStats.lanePam4Level0SD().value()) {
        printf("%4.2f    ", val);
      }
    }
    if (!hostPortStats.lanePam4Level1SD().value().empty()) {
      printf("\n  PAM4 L1 SD :  ");
      for (auto& [channel, val] : hostPortStats.lanePam4Level1SD().value()) {
        printf("%4.2f    ", val);
      }
    }
    if (!hostPortStats.lanePam4Level2SD().value().empty()) {
      printf("\n  PAM4 L2 SD:  ");
      for (auto& [channel, val] : hostPortStats.lanePam4Level2SD().value()) {
        printf("%4.2f    ", val);
      }
    }
    if (!hostPortStats.lanePam4Level3SD().value().empty()) {
      printf("\n  PAM4 L3 SD :  ");
      for (auto& [channel, val] : hostPortStats.lanePam4Level3SD().value()) {
        printf("%4.2f    ", val);
      }
    }
    if (!hostPortStats.lanePam4MPI().value().empty()) {
      printf("\n  PAM4 MPI   :  ");
      for (auto& [channel, val] : hostPortStats.lanePam4MPI().value()) {
        printf("%4.2f    ", val);
      }
    }
    if (!hostPortStats.lanePam4LTP().value().empty()) {
      printf("\n  PAM4 LTP   :  ");
      for (auto& [channel, val] : hostPortStats.lanePam4LTP().value()) {
        printf("%4.2f  ", val);
      }
    }
  }
  printf(
      "\nTime Collected: %s\n",
      getLocalTime(vdmStatsOpt.value().statsCollectionTme().value()).c_str());
  return true;
}

/*
 * printVdmInfoDirect
 *
 * Read the VDM information from the optics and print. The VDM config is
 * present in page 20 (2 bytes per config entry) and the VDM values are present
 * in page 24 (2 bytes value).
 */
bool printVdmInfoDirect(DirectI2cInfo i2cInfo, unsigned int port) {
  DOMDataUnion domDataUnion;

  printf("Displaying VDM info for module %d directly:\n", port);

  // Read the optics data directly from this process
  try {
    domDataUnion = fetchDataFromLocalI2CBus(i2cInfo, port);
  } catch (const std::exception& ex) {
    fprintf(stderr, "error reading QSFP data %u: %s\n", port, ex.what());
    return false;
  }

  if (domDataUnion.getType() != DOMDataUnion::Type::cmis) {
    printf("The VDM feature is available for CMIS optics only\n");
    return false;
  }

  CmisData cmisData = domDataUnion.get_cmis();
  auto page01Buf = can_throw(cmisData.page01())->data();
  auto page20Buf = can_throw(cmisData.page20())->data();
  auto page24Buf = can_throw(cmisData.page24())->data();
  auto page21Buf = can_throw(cmisData.page21())->data();
  auto page25Buf = can_throw(cmisData.page25())->data();

  // Check for VDM presence first
  if (!(page01Buf[14] & 0x40)) {
    fprintf(stderr, "VDM not supported on this module %d\n", port);
    return false;
  }

  // Go over the list. Page 0x20/0x21 contains the config and page 0x24/0x25
  // contains the values

  auto findAndPrintVdmInfo = [](const uint8_t* controlPageBuf,
                                const uint8_t* dataPageBuf) {
    for (int i = 0; i < 127; i += 2) {
      uint8_t byteMsb = controlPageBuf[i];
      uint8_t byteLsb = controlPageBuf[i + 1];

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
        printf("Lane %d %s = ", laneId, vdmConfigInfo.description.c_str());
      }

      // Get the VDM value corresponding to config type and convert it
      // appropriately. The U16 value will be = byte0 + byte1 * lsbScale For
      // F16 value:
      //   exponent = (byte0 >> 3) - 24
      //   mantissa = (byte0 & 0x7) << 8 | byte1
      //   value = mantissa * 10^exponent
      double vdmValue;
      if (vdmConfigInfo.dataType == VDM_DATA_TYPE_U16) {
        vdmValue =
            dataPageBuf[i] + (dataPageBuf[i + 1] * vdmConfigInfo.lsbScale);
        printf("%.2f %s\n", vdmValue, vdmConfigInfo.unit.c_str());
      } else if (vdmConfigInfo.dataType == VDM_DATA_TYPE_F16) {
        int expon = dataPageBuf[i] >> 3;
        int mant = ((dataPageBuf[i] & 0x7) << 8) | dataPageBuf[i + 1];
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

  uint8_t vdmGroupsSupported;
  readRegister(port, 128, 1, 0x2f, &vdmGroupsSupported);

  // Check for VDM Advance Group3 support
  // Control page: 0x22, data page: 0x26
  if ((vdmGroupsSupported & 0x03) >= 2) {
    auto page22Buf = can_throw(cmisData.page22())->data();
    auto page26Buf = can_throw(cmisData.page26())->data();
    findAndPrintVdmInfo(page22Buf, page26Buf);
  }

  return true;
}

/*
 * printVdmInfo
 *
 * Get and print the VDM performance monitoring diagsnostic data either
 * directly from hardware or through qsfp_service
 */
bool printVdmInfo(DirectI2cInfo i2cInfo, unsigned int port) {
  if (!FLAGS_direct_i2c) {
    if (QsfpServiceDetector::getInstance()->isQsfpServiceActive()) {
      return printVdmInfoViaService(port);
    } else {
      printf("Qsfp service is not active, pl provide --direct_i2c option\n");
      return false;
    }
  } else {
    return printVdmInfoDirect(i2cInfo, port);
  }
}

/*
 * printDiagsInfo
 *
 * Prints the diagnostic information for all the modules
 */
bool printDiagsInfo(folly::EventBase& evb) {
  DOMDataUnion domDataUnion;

  if (FLAGS_direct_i2c) {
    printf(
        "This command reads from qsfp_service so you can't use --direct_i2c option\n");
    return false;
  }
  printf("Displaying Diagnostic info for modules\n");

  auto returnedModulesInfo =
      fetchInfoFromQsfpService(std::vector<int32_t>{}, evb);
  printf(
      "Mod   Diag   VDM   CDB   PRBS_Line PRBS_Sys Lpbk_Line Lpbk_Sys TxDis RxDis SNR_Line SNR_Sys\n");

  for (auto& moduleInfo : returnedModulesInfo) {
    if (!moduleInfo.second.tcvrState().value().present().value()) {
      continue;
    }

    printf("%2d  ", moduleInfo.first);
    auto& diagCap =
        moduleInfo.second.tcvrState().value().diagCapability().value();
    printf("%5s", *diagCap.diagnostics() ? "Y" : "N");
    printf("%6s", *diagCap.vdm() ? "Y" : "N");
    printf("%6s", *diagCap.cdb() ? "Y" : "N");
    printf("%9s", *diagCap.prbsLine() ? "Y" : "N");
    printf("%10s", *diagCap.prbsSystem() ? "Y" : "N");
    printf("%9s", *diagCap.loopbackLine() ? "Y" : "N");
    printf("%10s", *diagCap.loopbackSystem() ? "Y" : "N");
    printf("%7s", *diagCap.txOutputControl() ? "Y" : "N");
    printf("%6s", *diagCap.rxOutputControl() ? "Y" : "N");
    printf("%7s", *diagCap.snrLine() ? "Y" : "N");
    printf("%9s", *diagCap.snrSystem() ? "Y" : "N");
    printf("\n");
  }
  return true;
}

/*
 * getEepromCsumStatus
 *
 * Print the EEPROM register checksum status. The checksum validation is
 * done over page data obtained in DomDataUnion
 */
bool getEepromCsumStatus(const DOMDataUnion& domDataUnion) {
  uint8_t computedCsum, recordedCsum;
  bool checkSumGood = true;

  auto get8BitCsum = [](const IOBuf& buf, int start, int len) -> uint8_t {
    if (start + len > buf.length()) {
      throw FbossError("Wrong eeprom range specified for checksum");
    }
    auto data = buf.data() + start;
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
      sum += data[i];
    }
    return sum;
  };

  if (domDataUnion.getType() == DOMDataUnion::Type::sff8636) {
    Sff8636Data sffData = domDataUnion.get_sff8636();
    auto page0IOBuf = sffData.page0().value();
    auto page0Buf = page0IOBuf.data();

    // Page0: csum(Reg_128..190) => Reg_191)
    computedCsum = get8BitCsum(page0IOBuf, 0, 63);
    recordedCsum = page0Buf[63];
    if (FLAGS_verbose) {
      printf(
          "    Page0 checksum = %s\n",
          (computedCsum == recordedCsum) ? "Valid" : "Invalid");
    }
    checkSumGood = checkSumGood && (computedCsum == recordedCsum);

    // Page0: csum(Reg_192..222) => Reg_223)
    computedCsum = get8BitCsum(page0IOBuf, 64, 31);
    recordedCsum = page0Buf[95];
    if (FLAGS_verbose) {
      printf(
          "    Page0Ext checksum = %s\n",
          (computedCsum == recordedCsum) ? "Valid" : "Invalid");
    }
    checkSumGood = checkSumGood && (computedCsum == recordedCsum);

  } else if (domDataUnion.getType() == DOMDataUnion::Type::cmis) {
    CmisData cmisData = domDataUnion.get_cmis();
    auto lowerPageBuf = cmisData.lower()->data();
    bool flatMem = ((lowerPageBuf[2] & 0x80) != 0);

    auto page0IOBuf = cmisData.page0().value();
    auto page0Buf = page0IOBuf.data();

    // Page0: csum(Reg_128..221) => Reg_222)
    computedCsum = get8BitCsum(page0IOBuf, 0, 94);
    recordedCsum = page0Buf[94];
    if (FLAGS_verbose) {
      printf(
          "    Page0 checksum = %s\n",
          (computedCsum == recordedCsum) ? "Valid" : "Invalid");
    }
    checkSumGood = checkSumGood && (computedCsum == recordedCsum);

    if (!flatMem) {
      auto page01IOBuf = can_throw(cmisData.page01()).value();
      auto page01Buf = page01IOBuf.data();
      auto page02IOBuf = can_throw(cmisData.page02()).value();
      auto page02Buf = page02IOBuf.data();

      // Page1: csum(Reg_130..254) => Reg_255)
      computedCsum = get8BitCsum(page01IOBuf, 2, 125);
      recordedCsum = page01Buf[127];
      if (FLAGS_verbose) {
        printf(
            "    Page1 checksum = %s\n",
            (computedCsum == recordedCsum) ? "Valid" : "Invalid");
      }
      checkSumGood = checkSumGood && (computedCsum == recordedCsum);

      // Page2: csum(Reg_128..254) => Reg_255)
      computedCsum = get8BitCsum(page02IOBuf, 0, 127);
      recordedCsum = page02Buf[127];
      if (FLAGS_verbose) {
        printf(
            "    Page2 checksum = %s\n",
            (computedCsum == recordedCsum) ? "Valid" : "Invalid");
      }
      checkSumGood = checkSumGood && (computedCsum == recordedCsum);
    }
  } else {
    fprintf(stderr, "Unrecognizable DOMDataUnion format.\n");
    return false;
  }

  return checkSumGood;
}

/*
 * setModulePrbs
 *
 * Sets the module PRBS state. This can work with either qsfp_service or use
 * direct_i2c calls
 */
void setModulePrbs(
    DirectI2cInfo i2cInfo,
    std::vector<std::string> portList,
    bool start) {
  if (QsfpServiceDetector::getInstance()->isQsfpServiceActive()) {
    folly::EventBase& evb = QsfpUtilContainer::getInstance()->getEventBase();
    setModulePrbsViaService(evb, portList, start);
  } else {
    setModulePrbsDirect(i2cInfo, portList, start);
  }
}

/*
 * setModulePrbsViaService
 *
 * Starts or stops the PRBS generator and checker on a module. This function
 * needs qsfp_service running as it makes thrift call to it for setting PRBS
 * mode on a module (or list of ports). This functiuon supports PBS31Q only.
 */
void setModulePrbsViaService(
    folly::EventBase& evb,
    std::vector<std::string> portList,
    bool start) {
  prbs::InterfacePrbsState prbsState;

  prbsState.polynomial() = nameToEnum<prbs::PrbsPolynomial>(FLAGS_prbs_pattern);
  if (!FLAGS_generator && !FLAGS_checker) {
    prbsState.generatorEnabled() = start;
    prbsState.checkerEnabled() = start;
  }
  if (FLAGS_generator) {
    prbsState.generatorEnabled() = start;
  }
  if (FLAGS_checker) {
    prbsState.checkerEnabled() = start;
  }

  auto client = getQsfpClient(evb);
  for (auto port : portList) {
    client->sync_setInterfacePrbs(
        port, phy::PortComponent::TRANSCEIVER_LINE, prbsState);
  }
}

/*
 * setModulePrbsDirect
 *
 * Sets the module PRBS state by using direct i2c operation. This function does
 * not know about the port speed and profile so it operates on transceiver
 * module rather than SW port inside the module
 */
void setModulePrbsDirect(
    DirectI2cInfo i2cInfo,
    std::vector<std::string> portList,
    bool start) {
  auto bus = i2cInfo.bus;
  auto wedgeManager = i2cInfo.transceiverManager;

  for (auto& portName : portList) {
    auto module = wedgeManager->getPortNameToModuleMap().at(portName) + 1;
    auto managementInterface = getModuleTypeDirect(bus, module);
    if (managementInterface == TransceiverManagementInterface::CMIS) {
      setModulePrbsDirectCmis(bus, module, start);
    }
  }
}

/*
 * setModulePrbsDirectCmis
 *
 * Sets/resets the PRBS generator and/or checker on a CMIS module
 */
void setModulePrbsDirectCmis(TransceiverI2CApi* bus, int module, bool start) {
  // Set the page first
  uint8_t prbsPage = 0x13;
  bus->moduleWrite(
      module, {TransceiverAccessParameter::ADDR_QSFP, 127, 1}, &prbsPage);

  auto patternEnum = nameToEnum<prbs::PrbsPolynomial>(FLAGS_prbs_pattern);
  auto patternInt = cmisPrbsPolynominalMap[patternEnum];

  // Set the PRBS generator and/or checker pattern type, default - PRBS31Q
  if (start) {
    uint8_t pattern[4];
    for (int i = 0; i < 4; i++) {
      pattern[i] = ((patternInt & 0xf) << 4) | (patternInt & 0xf);
    }
    if (FLAGS_generator) {
      bus->moduleWrite(
          module, {TransceiverAccessParameter::ADDR_QSFP, 156, 4}, pattern);
    }
    if (FLAGS_checker) {
      bus->moduleWrite(
          module, {TransceiverAccessParameter::ADDR_QSFP, 172, 4}, pattern);
    }
  }

  // Set/Reset the PRBS generator and/or checker
  if (FLAGS_generator) {
    uint8_t generator = start ? 0xff : 0x00;
    bus->moduleWrite(
        module, {TransceiverAccessParameter::ADDR_QSFP, 152, 1}, &generator);
  }
  if (FLAGS_checker) {
    uint8_t checker = start ? 0xff : 0x00;
    bus->moduleWrite(
        module, {TransceiverAccessParameter::ADDR_QSFP, 168, 1}, &checker);
  }
}

/*
 * getModulePrbsStatus
 *
 * This function gets the PRBS stats from the module and prints it. It either
 * gets data from qsfp_service or uses direct I2C operations
 */
void getModulePrbsStats(DirectI2cInfo i2cInfo, std::vector<PortID> portList) {
  if (QsfpServiceDetector::getInstance()->isQsfpServiceActive()) {
    folly::EventBase& evb = QsfpUtilContainer::getInstance()->getEventBase();
    getModulePrbsStatsViaService(evb, portList);
  } else {
    getModulePrbsStatsDirect(i2cInfo, portList);
  }
}

/*
 * getModulePrbsStatsViaService
 *
 * Prints the PRBS status (Lock status and BER values) for a module where PRBS
 * is running. This function fetches data from qsfp_service
 */
void getModulePrbsStatsViaService(
    folly::EventBase& evb,
    std::vector<PortID> portList) {
  auto client = getQsfpClient(evb);
  for (auto port : portList) {
    phy::PrbsStats prbsStats;
    client->sync_getPortPrbsStats(
        prbsStats, port, phy::PortComponent::TRANSCEIVER_LINE);

    printf(
        "PRBS time collected: %s\n",
        getLocalTime(*prbsStats.timeCollected()).c_str());
    for (auto laneStats : prbsStats.laneStats().value()) {
      printf("Lane: %d\n", laneStats.laneId().value());
      printf("  Locked: %s\n", (laneStats.locked().value() ? "True" : "False"));
      printf("  BER: %e\n", laneStats.ber().value());
      printf("  Max BER: %e\n", laneStats.maxBer().value());
      printf(
          "  SNR: %f\n",
          (laneStats.snr().has_value() ? laneStats.snr().value() : 0));
      printf(
          "  Max SNR: %f\n",
          (laneStats.maxSnr().has_value() ? laneStats.maxSnr().value() : 0));
      printf("  Num Loss of lock: %d\n", laneStats.numLossOfLock().value());
    }
  }
}

/*
 * getModulePrbsStatsDirect
 *
 * This function prints the PRBS stats for the module. This function does not
 * know about the port speed and profile so it operates on transceiver module
 * rather than SW port inside the module
 */
void getModulePrbsStatsDirect(
    DirectI2cInfo i2cInfo,
    const std::vector<PortID>& portList) {
  auto bus = i2cInfo.bus;
  auto wedgeManager = i2cInfo.transceiverManager;

  std::set<int> modules;
  for (auto portId : portList) {
    auto module = wedgeManager->getTransceiverID(portId);
    if (!module.has_value()) {
      continue;
    }
    modules.insert(module.value() + 1);
  }

  for (auto module : modules) {
    auto managementInterface = getModuleTypeDirect(bus, module);
    if (managementInterface == TransceiverManagementInterface::CMIS) {
      getModulePrbsStatsDirectCmis(bus, module);
    }
  }
}

/*
 * getModulePrbsStatsDirectCmis
 *
 * Reads and prints the module PRBS stats for a CMIS module
 */
void getModulePrbsStatsDirectCmis(TransceiverI2CApi* bus, int module) {
  // Set the page first
  uint8_t prbsPage = 0x14;
  bus->moduleWrite(
      module, {TransceiverAccessParameter::ADDR_QSFP, 127, 1}, &prbsPage);

  // Read the PRBS generator and checker lock status
  std::array<bool, 8> prbsGeneratorLocked;
  std::array<bool, 8> prbsCheckerLocked;

  uint8_t generatorLol;
  bus->moduleRead(
      module, {TransceiverAccessParameter::ADDR_QSFP, 137, 1}, &generatorLol);
  for (int i = 0; i < 8; i++) {
    prbsGeneratorLocked[i] = !((generatorLol >> i) & 0x1);
  }

  uint8_t checkerLol;
  bus->moduleRead(
      module, {TransceiverAccessParameter::ADDR_QSFP, 139, 1}, &checkerLol);
  for (int i = 0; i < 8; i++) {
    prbsCheckerLocked[i] = !((checkerLol >> i) & 0x1);
  }

  // Read BER values
  std::array<float, 8> mediaBer;

  uint8_t diagSelect = 1;
  bus->moduleWrite(
      module, {TransceiverAccessParameter::ADDR_QSFP, 128, 1}, &diagSelect);
  // Some modules take 1.8s to populate BER
  usleep(2 * 1000 * 1000); // @lint-ignore CLANGTIDY

  std::array<uint8_t, 16> berData;
  bus->moduleRead(
      module, {TransceiverAccessParameter::ADDR_QSFP, 208, 16}, berData.data());

  for (int i = 0; i < 8; i++) {
    int exponent = berData[i * 2] >> 3;
    exponent -= 24;
    int mantissa = ((berData[i * 2] & 0x7) << 8) | berData[(i * 2) + 1];
    mediaBer[i] = mantissa * exp10(exponent);
  }

  // Read SNR values
  std::array<float, 8> mediaSnr;

  diagSelect = 6;
  bus->moduleWrite(
      module, {TransceiverAccessParameter::ADDR_QSFP, 128, 1}, &diagSelect);
  usleep(2 * 1000 * 1000); // @lint-ignore CLANGTIDY

  std::array<uint8_t, 16> snrData;
  bus->moduleRead(
      module, {TransceiverAccessParameter::ADDR_QSFP, 240, 16}, snrData.data());

  for (int i = 0; i < 8; i++) {
    mediaSnr[i] = snrData[i * 2] / 256.0 + snrData[(i * 2) + 1];
  }

  // Print PRBS data
  printf("Module %d: PRBS stats\n", module);
  printf(
      "Lane      GeneratorLock      CheckerLock      MediaBER       MediaSNR\n");
  for (int i = 0; i < 8; i++) {
    printf(
        "%1d               %1s                 %1s           %.2e        %.2f\n",
        i,
        (prbsGeneratorLocked[i] ? "Y" : "N"),
        (prbsCheckerLocked[i] ? "Y" : "N"),
        mediaBer[i],
        mediaSnr[i]);
  }
}

/*
 * Verify the select command is working properly with regard to the
 * --direct-i2c option
 */
bool verifyDirectI2cCompliance() {
  // TODO (sampham): the commands in the if statement below are working
  // properly with regard to the --direct-i2c option. After the remaining
  // commands do the same, delete the if statement below. The if statement
  // below allows an incremental addition of properly behaved commands to
  // force users to use them correctly right away without having to wait for
  // all commands to be compliant at once
  if (FLAGS_tx_disable || FLAGS_tx_enable || FLAGS_pause_remediation ||
      FLAGS_get_remediation_until_time || FLAGS_read_reg || FLAGS_write_reg ||
      FLAGS_update_module_firmware || FLAGS_update_bulk_module_fw ||
      FLAGS_set_40g || FLAGS_set_100g || FLAGS_electrical_loopback ||
      FLAGS_optical_loopback || FLAGS_clear_loopback || FLAGS_qsfp_reset) {
    if (FLAGS_direct_i2c) {
      if (QsfpServiceDetector::getInstance()->isQsfpServiceActive()) {
        XLOG(ERR)
            << "--direct_i2c option is used while QSFP service is running. Please stop QSFP service first or do not use the --direct_i2c option";
        return false;
      }

      // It does not make sense to use the below commands while qsfp_service
      // is not running
      if (FLAGS_pause_remediation || FLAGS_get_remediation_until_time) {
        XLOG(ERR) << "This command does not support the --direct_i2c option";
        return false;
      }
    } else {
      if (!QsfpServiceDetector::getInstance()->isQsfpServiceActive()) {
        XLOG(ERR)
            << "--direct_i2c option is not used while QSFP service is not running. Please run QSFP service first or use the --direct_i2c option";
        return false;
      }

      // The below commands work with the --direct-i2c option only
      if (FLAGS_update_module_firmware || FLAGS_update_bulk_module_fw) {
        XLOG(ERR)
            << "This command is expected to be used with the --direct_i2c option";
        return false;
      }
    }
  }

  return true;
}

void printModuleTransactionStats(
    const std::vector<int32_t>& ports,
    folly::EventBase& evb) {
  int64_t totalRead = 0, readFailed = 0, totalWrite = 0, writeFailed = 0;
  float readFailure, writeFailure;
  auto modulesInfo = fetchInfoFromQsfpService(ports, evb);

  printf(
      "Module        ReadsAttempted      ReadFailed(%%)   WritesAttempted    WritesFailed(%%)\n");
  for (auto& moduleInfo : modulesInfo) {
    if (moduleInfo.second.tcvrStats()->stats().has_value()) {
      auto& moduleStat = moduleInfo.second.tcvrStats()->stats().value();
      readFailure = moduleStat.numReadAttempted().value()
          ? (double(moduleStat.numReadFailed().value() * 100) /
             moduleStat.numReadAttempted().value())
          : 0;
      writeFailure = moduleStat.numWriteAttempted().value()
          ? (double(moduleStat.numWriteFailed().value() * 100) /
             moduleStat.numWriteAttempted().value())
          : 0;

      printf(
          "%3d         %10ld      %10ld (%5.2f%%)  %10ld     %10ld (%5.2f%%))\n",
          moduleInfo.first,
          moduleStat.numReadAttempted().value(),
          moduleStat.numReadFailed().value(),
          readFailure,
          moduleStat.numWriteAttempted().value(),
          moduleStat.numWriteFailed().value(),
          writeFailure);

      totalRead += moduleStat.numReadAttempted().value();
      readFailed += moduleStat.numReadFailed().value();
      totalWrite += moduleStat.numWriteAttempted().value();
      writeFailed += moduleStat.numWriteFailed().value();
    }
  }
  readFailure = totalRead ? (double(readFailed * 100) / totalRead) : 0;
  writeFailure = totalWrite ? (double(writeFailed * 100) / totalWrite) : 0;
  printf(
      "Total       %10ld      %10ld (%5.2f%%)  %10ld     %10ld (%5.2f%%)\n",
      totalRead,
      readFailed,
      readFailure,
      totalWrite,
      writeFailed,
      writeFailure);
}

} // namespace facebook::fboss
