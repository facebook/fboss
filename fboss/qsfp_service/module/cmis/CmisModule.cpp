// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/cmis/CmisModule.h"

#include <boost/assign.hpp>
#include <boost/bimap.hpp>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <array>
#include <cmath>
#include <string>
#include "common/time/Time.h"
#include "fboss/agent/FbossError.h"
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/lib/QsfpConfigParserHelper.h"
#include "fboss/qsfp_service/module/FirmwareUpgrader.h"
#include "fboss/qsfp_service/module/QsfpFieldInfo.h"
#include "fboss/qsfp_service/module/QsfpHelper.h"
#include "fboss/qsfp_service/module/TransceiverImpl.h"
#include "fboss/qsfp_service/module/cmis/CmisFieldInfo.h"
#include "fboss/qsfp_service/module/cmis/CmisHelper.h"

#include <thrift/lib/cpp/util/EnumUtils.h>

using folly::IOBuf;
using std::lock_guard;
using std::memcpy;
using std::mutex;
using namespace apache::thrift;

DEFINE_bool(
    set_max_fec_sampling,
    false,
    "Flag to enable setting max FEC sampling for module");

namespace {

constexpr int kUsecBetweenPowerModeFlap = 100000;
constexpr int kUsecBetweenLaneInit = 10000;
constexpr int kUsecVdmLatchHold = 100000;
constexpr int kUsecDiagSelectLatchWait = 200000;
constexpr int kUsecAfterAppProgramming = 500000;
constexpr int kUsecDatapathStateUpdateTime = 10000000; // 10 seconds
// We may need special handling for scenarios where Init time takes
// more than 120 seconds. we will likely need to refactor code.
constexpr int kUsecDatapathStateUpdateTimeMaxFboss = 120000000; // 120 seconds
constexpr int kUsecDatapathStatePollTime = 500000; // 500 ms
constexpr double kU16TypeLsbDivisor = 256.0;
constexpr int kVdmDescriptorLength = 2;
constexpr int kFR4LiteSMFLength = 500; // 500 meters

// Definitions for CDB Histogram
constexpr int kCdbSymErrHistBinSize = 6;
constexpr int kCdbSymErrHistMaxOffset = 1;
constexpr int kCdbSymErrHistAvgOffset = 3;
constexpr int kCdbSymErrHistCurOffset = 5;

constexpr int kMaxFecTailRs544 = 15;

// Datapath init/deinit variables
constexpr uint8_t DP_INIT_MAX_MASK = 0x0F;
constexpr uint8_t DP_DINIT_MAX_MASK = 0xF0;
constexpr uint8_t DP_DINIT_BITSHIFT = 4;

// Tunable module const expr
constexpr double kMhzToGhzFactor = 0.000001;

// TODO @sanabani: Change To Map
std::array<std::string, 9> channelConfigErrorMsg = {
    "No status available, config under progress",
    "Config accepted and applied",
    "Config rejected due to unknown reason",
    "Config rejected due to invalid ApSel code request",
    "Config rejected due to ApSel requested on invalid lane combination",
    "Config rejected due to invalid SI control set request",
    "Config rejected due to some lanes currently in use by other application",
    "Config rejected due to incomplete lane info",
    "Config rejected due to other reasons"};

} // namespace

namespace facebook {
namespace fboss {

using namespace facebook::fboss::phy;

enum DiagnosticFeatureEncoding {
  NONE = 0x0,
  BER = 0x1,
  SNR = 0x6,
  LATCHED_BER = 0x11,
};

// Datapath init/deinit variables
static const std::unordered_map<uint8_t, uint64_t> DpInitValToTimeMap = {
    {0, 1000}, // Tstate < 1 ms
    {1, 5000}, // 1 ms <= Tstate < 5 ms
    {2, 10000}, // 5 ms <= Tstate < 10 ms
    {3, 50000}, // 10 ms <= Tstate < 50 ms
    {4, 100000}, // 50 ms <= Tstate < 100 ms
    {5, 500000}, // 100 ms <= Tstate < 500 ms
    {6, 1000000}, // 500 ms <= Tstate < 1 s
    {7, 5000000}, // 1 s <= Tstate < 5 s
    {8, 10000000}, // 5 s <= Tstate < 10 s
    {9, 60000000}, // 10 s <= Tstate < 1 min
    {10, 300000000}, // 1 min <= Tstate < 5 min
    {11, 600000000}, // 5 min <= Tstate < 10 min
    {12, 3000000000}, // 10 min <= Tstate < 50 min
};

// As per CMIS4.0
static const QsfpFieldInfo<CmisField, CmisPages>::QsfpFieldMap cmisFields = {
    // Lower Page
    {CmisField::PAGE_LOWER, {CmisPages::LOWER, 0, 128}},
    {CmisField::IDENTIFIER, {CmisPages::LOWER, 0, 1}},
    {CmisField::REVISION_COMPLIANCE, {CmisPages::LOWER, 1, 1}},
    {CmisField::FLAT_MEM, {CmisPages::LOWER, 2, 1}},
    {CmisField::MODULE_STATE, {CmisPages::LOWER, 3, 1}},
    {CmisField::BANK0_FLAGS, {CmisPages::LOWER, 4, 1}},
    {CmisField::BANK1_FLAGS, {CmisPages::LOWER, 5, 1}},
    {CmisField::BANK2_FLAGS, {CmisPages::LOWER, 6, 1}},
    {CmisField::BANK3_FLAGS, {CmisPages::LOWER, 7, 1}},
    {CmisField::MODULE_FLAG, {CmisPages::LOWER, 8, 1}},
    {CmisField::MODULE_ALARMS, {CmisPages::LOWER, 9, 3}},
    {CmisField::TEMPERATURE, {CmisPages::LOWER, 14, 2}},
    {CmisField::VCC, {CmisPages::LOWER, 16, 2}},
    {CmisField::MODULE_CONTROL, {CmisPages::LOWER, 26, 1}},
    {CmisField::FIRMWARE_REVISION, {CmisPages::LOWER, 39, 2}},
    {CmisField::FEC_SAMPLING_PCT, {CmisPages::LOWER, 65, 1}},
    {CmisField::MEDIA_TYPE_ENCODINGS, {CmisPages::LOWER, 85, 1}},
    {CmisField::APPLICATION_ADVERTISING1, {CmisPages::LOWER, 86, 4}},
    {CmisField::BANK_SELECT, {CmisPages::LOWER, 126, 1}},
    {CmisField::PAGE_SELECT_BYTE, {CmisPages::LOWER, 127, 1}},
    // Page 00h
    {CmisField::PAGE_UPPER00H, {CmisPages::PAGE00, 128, 128}},
    {CmisField::VENDOR_NAME, {CmisPages::PAGE00, 129, 16}},
    {CmisField::VENDOR_OUI, {CmisPages::PAGE00, 145, 3}},
    {CmisField::PART_NUMBER, {CmisPages::PAGE00, 148, 16}},
    {CmisField::REVISION_NUMBER, {CmisPages::PAGE00, 164, 2}},
    {CmisField::VENDOR_SERIAL_NUMBER, {CmisPages::PAGE00, 166, 16}},
    {CmisField::MFG_DATE, {CmisPages::PAGE00, 182, 8}},
    {CmisField::LENGTH_COPPER, {CmisPages::PAGE00, 202, 1}},
    {CmisField::MEDIA_INTERFACE_TECHNOLOGY, {CmisPages::PAGE00, 212, 1}},
    {CmisField::PAGE0_CSUM, {CmisPages::PAGE00, 222, 1}},
    // Page 01h
    {CmisField::PAGE_UPPER01H, {CmisPages::PAGE01, 128, 128}},
    {CmisField::LENGTH_SMF, {CmisPages::PAGE01, 132, 1}},
    {CmisField::LENGTH_OM5, {CmisPages::PAGE01, 133, 1}},
    {CmisField::LENGTH_OM4, {CmisPages::PAGE01, 134, 1}},
    {CmisField::LENGTH_OM3, {CmisPages::PAGE01, 135, 1}},
    {CmisField::LENGTH_OM2, {CmisPages::PAGE01, 136, 1}},
    {CmisField::VDM_DIAG_SUPPORT, {CmisPages::PAGE01, 142, 1}},
    {CmisField::MAX_DPINIT_TIME, {CmisPages::PAGE01, 144, 1}},
    {CmisField::TX_CONTROL_SUPPORT, {CmisPages::PAGE01, 155, 1}},
    {CmisField::RX_CONTROL_SUPPORT, {CmisPages::PAGE01, 156, 1}},
    {CmisField::TX_BIAS_MULTIPLIER, {CmisPages::PAGE01, 160, 1}},
    {CmisField::TX_SIG_INT_CONT_AD, {CmisPages::PAGE01, 161, 1}},
    {CmisField::RX_SIG_INT_CONT_AD, {CmisPages::PAGE01, 162, 1}},
    {CmisField::CDB_SUPPORT, {CmisPages::PAGE01, 163, 1}},
    {CmisField::MEDIA_LANE_ASSIGNMENT, {CmisPages::PAGE01, 176, 15}},
    {CmisField::DSP_FW_VERSION, {CmisPages::PAGE01, 194, 2}},
    {CmisField::BUILD_REVISION, {CmisPages::PAGE01, 196, 2}},
    {CmisField::APPLICATION_ADVERTISING2, {CmisPages::PAGE01, 223, 4}},
    {CmisField::PAGE1_CSUM, {CmisPages::PAGE01, 255, 1}},
    // Page 02h
    {CmisField::PAGE_UPPER02H, {CmisPages::PAGE02, 128, 128}},
    {CmisField::TEMPERATURE_THRESH, {CmisPages::PAGE02, 128, 8}},
    {CmisField::VCC_THRESH, {CmisPages::PAGE02, 136, 8}},
    {CmisField::TX_PWR_THRESH, {CmisPages::PAGE02, 176, 8}},
    {CmisField::TX_BIAS_THRESH, {CmisPages::PAGE02, 184, 8}},
    {CmisField::RX_PWR_THRESH, {CmisPages::PAGE02, 192, 8}},
    {CmisField::PAGE2_CSUM, {CmisPages::PAGE02, 255, 1}},
    // Page 04h
    {CmisField::PAGE_UPPER04H, {CmisPages::PAGE04, 128, 128}},
    {CmisField::LASER_GRIDS_ADVER, {CmisPages::PAGE04, 128, 1}},
    {CmisField::FINE_TUNING_ADVER, {CmisPages::PAGE04, 129, 1}},
    {CmisField::LASER_3P125_GHZ_LO_CHAN, {CmisPages::PAGE04, 130, 2}},
    {CmisField::LASER_3P125_GHZ_HI_CHAN, {CmisPages::PAGE04, 132, 2}},
    {CmisField::LASER_6P25_GHZ_LO_CHAN, {CmisPages::PAGE04, 134, 2}},
    {CmisField::LASER_6P25_GHZ_HI_CHAN, {CmisPages::PAGE04, 136, 2}},
    {CmisField::LASER_12P5_GHZ_LO_CHAN, {CmisPages::PAGE04, 138, 2}},
    {CmisField::LASER_12P5_GHZ_HI_CHAN, {CmisPages::PAGE04, 140, 2}},
    {CmisField::LASER_25_GHZ_LO_CHAN, {CmisPages::PAGE04, 142, 2}},
    {CmisField::LASER_25_GHZ_HI_CHAN, {CmisPages::PAGE04, 144, 2}},
    {CmisField::LASER_50_GHZ_LO_CHAN, {CmisPages::PAGE04, 146, 2}},
    {CmisField::LASER_50_GHZ_HI_CHAN, {CmisPages::PAGE04, 148, 2}},
    {CmisField::LASER_100_GHZ_LO_CHAN, {CmisPages::PAGE04, 150, 2}},
    {CmisField::LASER_100_GHZ_HI_CHAN, {CmisPages::PAGE04, 152, 2}},
    {CmisField::LASER_33_GHZ_LO_CHAN, {CmisPages::PAGE04, 154, 2}},
    {CmisField::LASER_33_GHZ_HI_CHAN, {CmisPages::PAGE04, 156, 2}},
    {CmisField::LASER_75_GHZ_LO_CHAN, {CmisPages::PAGE04, 158, 2}},
    {CmisField::LASER_75_GHZ_HI_CHAN, {CmisPages::PAGE04, 160, 2}},
    {CmisField::LASER_150_GHZ_LO_CHAN, {CmisPages::PAGE04, 162, 2}},
    {CmisField::LASER_150_GHZ_HI_CHAN, {CmisPages::PAGE04, 164, 2}},
    {CmisField::LASER_FINE_TUNE_RES, {CmisPages::PAGE04, 190, 2}},
    {CmisField::LASER_FINE_TUNE_LO_OFFSET, {CmisPages::PAGE04, 192, 2}},
    {CmisField::LASER_FINE_TUNE_HI_OFFSET, {CmisPages::PAGE04, 194, 2}},
    {CmisField::MEDIA_TX_PROG_OUT_PWR_ADVER, {CmisPages::PAGE04, 196, 1}},
    {CmisField::MEDIA_MIN_TX_PROG_OUT_PWR, {CmisPages::PAGE04, 198, 2}},
    {CmisField::MEDIA_MAX_TX_PROG_OUT_PWR, {CmisPages::PAGE04, 200, 2}},
    {CmisField::PAGE4_CSUM, {CmisPages::PAGE04, 255, 1}},
    // Page 10h
    {CmisField::PAGE_UPPER10H, {CmisPages::PAGE10, 128, 128}},
    {CmisField::DATA_PATH_DEINIT, {CmisPages::PAGE10, 128, 1}},
    {CmisField::TX_POLARITY_FLIP, {CmisPages::PAGE10, 129, 1}},
    {CmisField::TX_DISABLE, {CmisPages::PAGE10, 130, 1}},
    {CmisField::TX_SQUELCH_DISABLE, {CmisPages::PAGE10, 131, 1}},
    {CmisField::TX_FORCE_SQUELCH, {CmisPages::PAGE10, 132, 1}},
    {CmisField::TX_ADAPTATION_FREEZE, {CmisPages::PAGE10, 134, 1}},
    {CmisField::TX_ADAPTATION_STORE, {CmisPages::PAGE10, 135, 2}},
    {CmisField::RX_POLARITY_FLIP, {CmisPages::PAGE10, 137, 1}},
    {CmisField::RX_DISABLE, {CmisPages::PAGE10, 138, 1}},
    {CmisField::RX_SQUELCH_DISABLE, {CmisPages::PAGE10, 139, 1}},
    {CmisField::STAGE_CTRL_SET_0, {CmisPages::PAGE10, 143, 1}},
    {CmisField::STAGE_CTRL_SET0_IMMEDIATE, {CmisPages::PAGE10, 144, 1}},
    {CmisField::APP_SEL_LANE_1_8, {CmisPages::PAGE10, 145, 8}},
    {CmisField::APP_SEL_LANE_1_2, {CmisPages::PAGE10, 145, 2}},
    {CmisField::APP_SEL_LANE_3_4, {CmisPages::PAGE10, 147, 2}},
    {CmisField::APP_SEL_LANE_5_6, {CmisPages::PAGE10, 149, 2}},
    {CmisField::APP_SEL_LANE_7_8, {CmisPages::PAGE10, 151, 2}},
    {CmisField::APP_SEL_LANE_1_4, {CmisPages::PAGE10, 145, 4}},
    {CmisField::APP_SEL_LANE_5_8, {CmisPages::PAGE10, 149, 4}},
    {CmisField::APP_SEL_LANE_1, {CmisPages::PAGE10, 145, 1}},
    {CmisField::APP_SEL_LANE_2, {CmisPages::PAGE10, 146, 1}},
    {CmisField::APP_SEL_LANE_3, {CmisPages::PAGE10, 147, 1}},
    {CmisField::APP_SEL_LANE_4, {CmisPages::PAGE10, 148, 1}},
    {CmisField::APP_SEL_LANE_5, {CmisPages::PAGE10, 149, 1}},
    {CmisField::APP_SEL_LANE_6, {CmisPages::PAGE10, 150, 1}},
    {CmisField::APP_SEL_LANE_7, {CmisPages::PAGE10, 151, 1}},
    {CmisField::APP_SEL_LANE_8, {CmisPages::PAGE10, 152, 1}},
    {CmisField::RX_CONTROL_PRE_CURSOR, {CmisPages::PAGE10, 162, 4}},
    {CmisField::RX_CONTROL_PRE_CURSOR_LANE_01, {CmisPages::PAGE10, 162, 1}},
    {CmisField::RX_CONTROL_PRE_CURSOR_LANE_23, {CmisPages::PAGE10, 163, 1}},
    {CmisField::RX_CONTROL_PRE_CURSOR_LANE_45, {CmisPages::PAGE10, 164, 1}},
    {CmisField::RX_CONTROL_PRE_CURSOR_LANE_67, {CmisPages::PAGE10, 165, 1}},
    {CmisField::RX_CONTROL_POST_CURSOR, {CmisPages::PAGE10, 166, 4}},
    {CmisField::RX_CONTROL_POST_CURSOR_LANE_01, {CmisPages::PAGE10, 166, 1}},
    {CmisField::RX_CONTROL_POST_CURSOR_LANE_23, {CmisPages::PAGE10, 167, 1}},
    {CmisField::RX_CONTROL_POST_CURSOR_LANE_45, {CmisPages::PAGE10, 168, 1}},
    {CmisField::RX_CONTROL_POST_CURSOR_LANE_67, {CmisPages::PAGE10, 169, 1}},
    {CmisField::RX_CONTROL_MAIN, {CmisPages::PAGE10, 170, 4}},
    {CmisField::RX_CONTROL_MAIN_LANE_01, {CmisPages::PAGE10, 170, 1}},
    {CmisField::RX_CONTROL_MAIN_LANE_23, {CmisPages::PAGE10, 171, 1}},
    {CmisField::RX_CONTROL_MAIN_LANE_45, {CmisPages::PAGE10, 172, 1}},
    {CmisField::RX_CONTROL_MAIN_LANE_67, {CmisPages::PAGE10, 173, 1}},
    // Page 11h
    {CmisField::PAGE_UPPER11H, {CmisPages::PAGE11, 128, 128}},
    {CmisField::DATA_PATH_STATE, {CmisPages::PAGE11, 128, 4}},
    {CmisField::TX_FAULT_FLAG, {CmisPages::PAGE11, 135, 1}},
    {CmisField::TX_LOS_FLAG, {CmisPages::PAGE11, 136, 1}},
    {CmisField::TX_LOL_FLAG, {CmisPages::PAGE11, 137, 1}},
    {CmisField::TX_EQ_FLAG, {CmisPages::PAGE11, 138, 1}},
    {CmisField::TX_PWR_FLAG, {CmisPages::PAGE11, 139, 4}},
    {CmisField::TX_BIAS_FLAG, {CmisPages::PAGE11, 143, 4}},
    {CmisField::RX_LOS_FLAG, {CmisPages::PAGE11, 147, 1}},
    {CmisField::RX_LOL_FLAG, {CmisPages::PAGE11, 148, 1}},
    {CmisField::RX_PWR_FLAG, {CmisPages::PAGE11, 149, 4}},
    {CmisField::CHANNEL_TX_PWR, {CmisPages::PAGE11, 154, 16}},
    {CmisField::CHANNEL_TX_BIAS, {CmisPages::PAGE11, 170, 16}},
    {CmisField::CHANNEL_RX_PWR, {CmisPages::PAGE11, 186, 16}},
    {CmisField::CONFIG_ERROR_LANES, {CmisPages::PAGE11, 202, 4}},
    {CmisField::ACTIVE_CTRL_ALL_LANES, {CmisPages::PAGE11, 206, 8}},
    {CmisField::ACTIVE_CTRL_LANE_1, {CmisPages::PAGE11, 206, 1}},
    {CmisField::ACTIVE_CTRL_LANE_2, {CmisPages::PAGE11, 207, 1}},
    {CmisField::ACTIVE_CTRL_LANE_3, {CmisPages::PAGE11, 208, 1}},
    {CmisField::ACTIVE_CTRL_LANE_4, {CmisPages::PAGE11, 209, 1}},
    {CmisField::ACTIVE_CTRL_LANE_5, {CmisPages::PAGE11, 210, 1}},
    {CmisField::ACTIVE_CTRL_LANE_6, {CmisPages::PAGE11, 211, 1}},
    {CmisField::ACTIVE_CTRL_LANE_7, {CmisPages::PAGE11, 212, 1}},
    {CmisField::ACTIVE_CTRL_LANE_8, {CmisPages::PAGE11, 213, 1}},
    {CmisField::TX_CDR_CONTROL, {CmisPages::PAGE11, 221, 1}},
    {CmisField::RX_CDR_CONTROL, {CmisPages::PAGE11, 222, 1}},
    {CmisField::RX_OUT_PRE_CURSOR, {CmisPages::PAGE11, 223, 4}},
    {CmisField::RX_OUT_POST_CURSOR, {CmisPages::PAGE11, 227, 4}},
    {CmisField::RX_OUT_MAIN, {CmisPages::PAGE11, 231, 4}},
    // Page 12h
    {CmisField::PAGE_UPPER12H, {CmisPages::PAGE12, 128, 128}},
    {CmisField::MEDIA_TX_1_GRID_AND_FINE_TUNE_ENA, {CmisPages::PAGE12, 128, 1}},
    {CmisField::MEDIA_TX_1_CHAN_NBR_SEL, {CmisPages::PAGE12, 136, 2}},
    {CmisField::MEDIA_TX_1_FINE_TUNE_FREQ_OFFSET, {CmisPages::PAGE12, 152, 2}},
    {CmisField::MEDIA_TX_1_CURR_LAS_FREQ, {CmisPages::PAGE12, 168, 4}},
    {CmisField::MEDIA_TX_1_TGT_OUTPUT_PWR, {CmisPages::PAGE12, 200, 2}},
    {CmisField::MEDIA_TX_1_LAS_STAT, {CmisPages::PAGE12, 222, 1}},
    {CmisField::MEDIA_TX_LAS_TUNE_SUM, {CmisPages::PAGE12, 230, 1}},
    {CmisField::MEDIA_TX_1_LAS_STAT_FLAGS, {CmisPages::PAGE12, 231, 1}},
    {CmisField::MEDIA_TX_1_LAS_STAT_MASKS, {CmisPages::PAGE12, 239, 1}},
    // Page 13h
    {CmisField::PAGE_UPPER13H, {CmisPages::PAGE13, 128, 128}},
    {CmisField::LOOPBACK_CAPABILITY, {CmisPages::PAGE13, 128, 1}},
    {CmisField::PATTERN_CAPABILITY, {CmisPages::PAGE13, 129, 1}},
    {CmisField::DIAGNOSTIC_CAPABILITY, {CmisPages::PAGE13, 130, 1}},
    {CmisField::PATTERN_CHECKER_CAPABILITY, {CmisPages::PAGE13, 131, 1}},
    {CmisField::HOST_SUPPORTED_GENERATOR_PATTERNS, {CmisPages::PAGE13, 132, 2}},
    {CmisField::MEDIA_SUPPORTED_GENERATOR_PATTERNS,
     {CmisPages::PAGE13, 134, 2}},
    {CmisField::HOST_SUPPORTED_CHECKER_PATTERNS, {CmisPages::PAGE13, 136, 2}},
    {CmisField::MEDIA_SUPPORTED_CHECKER_PATTERNS, {CmisPages::PAGE13, 138, 2}},
    {CmisField::HOST_GEN_ENABLE, {CmisPages::PAGE13, 144, 1}},
    {CmisField::HOST_GEN_INV, {CmisPages::PAGE13, 145, 1}},
    {CmisField::HOST_GEN_PRE_FEC, {CmisPages::PAGE13, 147, 1}},
    {CmisField::HOST_PATTERN_SELECT_LANE_2_1, {CmisPages::PAGE13, 148, 1}},
    {CmisField::HOST_PATTERN_SELECT_LANE_4_3, {CmisPages::PAGE13, 149, 1}},
    {CmisField::HOST_PATTERN_SELECT_LANE_6_5, {CmisPages::PAGE13, 150, 1}},
    {CmisField::HOST_PATTERN_SELECT_LANE_8_7, {CmisPages::PAGE13, 151, 1}},
    {CmisField::MEDIA_GEN_ENABLE, {CmisPages::PAGE13, 152, 1}},
    {CmisField::MEDIA_GEN_INV, {CmisPages::PAGE13, 153, 1}},
    {CmisField::MEDIA_GEN_PRE_FEC, {CmisPages::PAGE13, 155, 1}},
    {CmisField::MEDIA_PATTERN_SELECT_LANE_2_1, {CmisPages::PAGE13, 156, 1}},
    {CmisField::MEDIA_PATTERN_SELECT_LANE_4_3, {CmisPages::PAGE13, 157, 1}},
    {CmisField::MEDIA_PATTERN_SELECT_LANE_6_5, {CmisPages::PAGE13, 158, 1}},
    {CmisField::MEDIA_PATTERN_SELECT_LANE_8_7, {CmisPages::PAGE13, 159, 1}},
    {CmisField::HOST_CHECKER_ENABLE, {CmisPages::PAGE13, 160, 1}},
    {CmisField::HOST_CHECKER_INV, {CmisPages::PAGE13, 161, 1}},
    {CmisField::HOST_CHECKER_POST_FEC, {CmisPages::PAGE13, 163, 1}},
    {CmisField::HOST_CHECKER_PATTERN_SELECT_LANE_2_1,
     {CmisPages::PAGE13, 164, 1}},
    {CmisField::HOST_CHECKER_PATTERN_SELECT_LANE_4_3,
     {CmisPages::PAGE13, 165, 1}},
    {CmisField::HOST_CHECKER_PATTERN_SELECT_LANE_6_5,
     {CmisPages::PAGE13, 166, 1}},
    {CmisField::HOST_CHECKER_PATTERN_SELECT_LANE_8_7,
     {CmisPages::PAGE13, 167, 1}},
    {CmisField::MEDIA_CHECKER_ENABLE, {CmisPages::PAGE13, 168, 1}},
    {CmisField::MEDIA_CHECKER_INV, {CmisPages::PAGE13, 169, 1}},
    {CmisField::MEDIA_CHECKER_POST_FEC, {CmisPages::PAGE13, 171, 1}},
    {CmisField::MEDIA_CHECKER_PATTERN_SELECT_LANE_2_1,
     {CmisPages::PAGE13, 172, 1}},
    {CmisField::MEDIA_CHECKER_PATTERN_SELECT_LANE_4_3,
     {CmisPages::PAGE13, 173, 1}},
    {CmisField::MEDIA_CHECKER_PATTERN_SELECT_LANE_6_5,
     {CmisPages::PAGE13, 174, 1}},
    {CmisField::MEDIA_CHECKER_PATTERN_SELECT_LANE_8_7,
     {CmisPages::PAGE13, 175, 1}},
    {CmisField::REF_CLK_CTRL, {CmisPages::PAGE13, 176, 1}},
    {CmisField::BER_CTRL, {CmisPages::PAGE13, 177, 1}},
    {CmisField::HOST_NEAR_LB_EN, {CmisPages::PAGE13, 180, 1}},
    {CmisField::MEDIA_NEAR_LB_EN, {CmisPages::PAGE13, 181, 1}},
    {CmisField::HOST_FAR_LB_EN, {CmisPages::PAGE13, 182, 1}},
    {CmisField::MEDIA_FAR_LB_EN, {CmisPages::PAGE13, 183, 1}},
    {CmisField::REF_CLK_LOSS, {CmisPages::PAGE13, 206, 1}},
    {CmisField::HOST_CHECKER_GATING_COMPLETE, {CmisPages::PAGE13, 208, 1}},
    {CmisField::MEDIA_CHECKER_GATING_COMPLETE, {CmisPages::PAGE13, 209, 1}},
    {CmisField::HOST_PPG_LOL, {CmisPages::PAGE13, 210, 1}},
    {CmisField::MEDIA_PPG_LOL, {CmisPages::PAGE13, 211, 1}},
    {CmisField::HOST_BERT_LOL, {CmisPages::PAGE13, 212, 1}},
    {CmisField::MEDIA_BERT_LOL, {CmisPages::PAGE13, 213, 1}},
    // Page 14h
    {CmisField::PAGE_UPPER14H, {CmisPages::PAGE14, 128, 128}},
    {CmisField::DIAG_SEL, {CmisPages::PAGE14, 128, 1}},
    {CmisField::HOST_LANE_GENERATOR_LOL_LATCH, {CmisPages::PAGE14, 136, 1}},
    {CmisField::MEDIA_LANE_GENERATOR_LOL_LATCH, {CmisPages::PAGE14, 137, 1}},
    {CmisField::HOST_LANE_CHECKER_LOL_LATCH, {CmisPages::PAGE14, 138, 1}},
    {CmisField::MEDIA_LANE_CHECKER_LOL_LATCH, {CmisPages::PAGE14, 139, 1}},
    {CmisField::HOST_BER, {CmisPages::PAGE14, 192, 16}},
    {CmisField::MEDIA_BER_HOST_SNR, {CmisPages::PAGE14, 208, 16}},
    {CmisField::MEDIA_SNR, {CmisPages::PAGE14, 240, 16}},
    // Page 20h
    {CmisField::PAGE_UPPER20H, {CmisPages::PAGE20, 128, 128}},
    // Page 21h
    {CmisField::PAGE_UPPER21H, {CmisPages::PAGE21, 128, 128}},
    // Page 22h
    {CmisField::PAGE_UPPER22H, {CmisPages::PAGE22, 128, 128}},
    // Page 24h
    {CmisField::PAGE_UPPER24H, {CmisPages::PAGE24, 128, 128}},
    // Page 25h
    {CmisField::PAGE_UPPER25H, {CmisPages::PAGE25, 128, 128}},
    // Page 26h
    {CmisField::PAGE_UPPER26H, {CmisPages::PAGE26, 128, 128}},
    // Page 2Ch
    {CmisField::PAGE_UPPER2CH, {CmisPages::PAGE2C, 128, 128}},
    {CmisField::PAM4_MPI_ALARMS, {CmisPages::PAGE2C, 208, 4}},
    // Page 2Fh
    {CmisField::PAGE_UPPER2FH, {CmisPages::PAGE2F, 128, 128}},
    {CmisField::VDM_GROUPS_SUPPORT, {CmisPages::PAGE2F, 128, 1}},
    {CmisField::VDM_LATCH_REQUEST, {CmisPages::PAGE2F, 144, 1}},
    {CmisField::VDM_LATCH_DONE, {CmisPages::PAGE2F, 145, 1}},
};

CmisField laneToAppSelField(const std::set<uint8_t>& lanes) {
  const std::map<std::set<uint8_t>, CmisField> kLanesToCmisField = {
      {{0}, CmisField::APP_SEL_LANE_1},
      {{1}, CmisField::APP_SEL_LANE_2},
      {{2}, CmisField::APP_SEL_LANE_3},
      {{3}, CmisField::APP_SEL_LANE_4},
      {{4}, CmisField::APP_SEL_LANE_5},
      {{5}, CmisField::APP_SEL_LANE_6},
      {{6}, CmisField::APP_SEL_LANE_7},
      {{7}, CmisField::APP_SEL_LANE_8},
      {{0, 1}, CmisField::APP_SEL_LANE_1_2},
      {{2, 3}, CmisField::APP_SEL_LANE_3_4},
      {{4, 5}, CmisField::APP_SEL_LANE_5_6},
      {{6, 7}, CmisField::APP_SEL_LANE_7_8},
      {{0, 1, 2, 3}, CmisField::APP_SEL_LANE_1_4},
      {{4, 5, 6, 7}, CmisField::APP_SEL_LANE_5_8},
      {{0, 1, 2, 3, 4, 5, 6, 7}, CmisField::APP_SEL_LANE_1_8},
  };
  if (const auto& it = kLanesToCmisField.find(lanes);
      it != kLanesToCmisField.end()) {
    return it->second;
  }
  throw FbossError("Can't find app sel for lanes ", folly::join(",", lanes));
}

static std::unordered_map<int, CmisField> laneToActiveCtrlField = {
    {0, CmisField::ACTIVE_CTRL_LANE_1},
    {1, CmisField::ACTIVE_CTRL_LANE_2},
    {2, CmisField::ACTIVE_CTRL_LANE_3},
    {3, CmisField::ACTIVE_CTRL_LANE_4},
    {4, CmisField::ACTIVE_CTRL_LANE_5},
    {5, CmisField::ACTIVE_CTRL_LANE_6},
    {6, CmisField::ACTIVE_CTRL_LANE_7},
    {7, CmisField::ACTIVE_CTRL_LANE_8},
};

static CmisFieldMultiplier qsfpMultiplier = {
    {CmisField::LENGTH_SMF, 100},
    {CmisField::LENGTH_OM5, 2},
    {CmisField::LENGTH_OM4, 2},
    {CmisField::LENGTH_OM3, 2},
    {CmisField::LENGTH_OM2, 1},
    {CmisField::LENGTH_COPPER, 0.1},
};

// A map of programmable FEC sampling pct per Module Media type.
static const std::unordered_map<MediaInterfaceCode, uint8_t>
    kMaxProgFecSamplingSupportedMap_ = {
        {MediaInterfaceCode::FR4_2x400G, 20},
};

constexpr uint8_t kPage0CsumRangeStart = 128;
constexpr uint8_t kPage0CsumRangeLength = 94;
constexpr uint8_t kPage1CsumRangeStart = 130;
constexpr uint8_t kPage1CsumRangeLength = 125;
constexpr uint8_t kPage2CsumRangeStart = 128;
constexpr uint8_t kPage2CsumRangeLength = 127;

struct checksumInfoStruct {
  uint8_t checksumRangeStartOffset;
  uint8_t checksumRangeLength;
  CmisField checksumValOffset;
};

static std::map<CmisPages, checksumInfoStruct> checksumInfoCmis = {
    {CmisPages::PAGE00,
     {kPage0CsumRangeStart, kPage0CsumRangeLength, CmisField::PAGE0_CSUM}},
    {CmisPages::PAGE01,
     {kPage1CsumRangeStart, kPage1CsumRangeLength, CmisField::PAGE1_CSUM}},
    {CmisPages::PAGE02,
     {kPage2CsumRangeStart, kPage2CsumRangeLength, CmisField::PAGE2_CSUM}},
};

// Bidirectional map for storing the mapping of prbs polynomial to patternID in
// the spec
using PrbsMap = boost::bimap<prbs::PrbsPolynomial, uint32_t>;
// clang-format off
const PrbsMap prbsPatternMap = boost::assign::list_of<PrbsMap::relation>(
  prbs::PrbsPolynomial::PRBS7, 11)(
  prbs::PrbsPolynomial::PRBS9, 9)(
  prbs::PrbsPolynomial::PRBS13, 7)(
  prbs::PrbsPolynomial::PRBS15, 5)(
  prbs::PrbsPolynomial::PRBS23, 3)(
  prbs::PrbsPolynomial::PRBS31, 1)(
  prbs::PrbsPolynomial::PRBS7Q, 10)(
  prbs::PrbsPolynomial::PRBS9Q, 8)(
  prbs::PrbsPolynomial::PRBS13Q, 6)(
  prbs::PrbsPolynomial::PRBS15Q, 4)(
  prbs::PrbsPolynomial::PRBS23Q, 2)(
  prbs::PrbsPolynomial::PRBS31Q, 0)(
  prbs::PrbsPolynomial::PRBSSSPRQ, 12);
// clang-format on

std::array<CmisField, 4> prbsGenMediaPatternFields = {
    CmisField::MEDIA_PATTERN_SELECT_LANE_2_1,
    CmisField::MEDIA_PATTERN_SELECT_LANE_4_3,
    CmisField::MEDIA_PATTERN_SELECT_LANE_6_5,
    CmisField::MEDIA_PATTERN_SELECT_LANE_8_7};
std::array<CmisField, 4> prbsGenHostPatternFields = {
    CmisField::HOST_PATTERN_SELECT_LANE_2_1,
    CmisField::HOST_PATTERN_SELECT_LANE_4_3,
    CmisField::HOST_PATTERN_SELECT_LANE_6_5,
    CmisField::HOST_PATTERN_SELECT_LANE_8_7};

std::array<CmisField, 4> prbsChkMediaPatternFields = {
    CmisField::MEDIA_CHECKER_PATTERN_SELECT_LANE_2_1,
    CmisField::MEDIA_CHECKER_PATTERN_SELECT_LANE_4_3,
    CmisField::MEDIA_CHECKER_PATTERN_SELECT_LANE_6_5,
    CmisField::MEDIA_CHECKER_PATTERN_SELECT_LANE_8_7,
};
std::array<CmisField, 4> prbsChkHostPatternFields = {
    CmisField::HOST_CHECKER_PATTERN_SELECT_LANE_2_1,
    CmisField::HOST_CHECKER_PATTERN_SELECT_LANE_4_3,
    CmisField::HOST_CHECKER_PATTERN_SELECT_LANE_6_5,
    CmisField::HOST_CHECKER_PATTERN_SELECT_LANE_8_7,
};

// Each of the configuration byte controls 2 lanes. There are 4 bytes each for
// pre-cursor, post-cursor and main amplitude. In this map, key is 0-3 (one of
// the 4 bytes), and the value is a list of CmisField for settings corresponding
// to that index
std::map<int, std::vector<CmisField>> offsetIndexToCmisField = {
    {0,
     {CmisField::RX_CONTROL_PRE_CURSOR_LANE_01,
      CmisField::RX_CONTROL_POST_CURSOR_LANE_01,
      CmisField::RX_CONTROL_MAIN_LANE_01}},
    {1,
     {CmisField::RX_CONTROL_PRE_CURSOR_LANE_23,
      CmisField::RX_CONTROL_POST_CURSOR_LANE_23,
      CmisField::RX_CONTROL_MAIN_LANE_23}},
    {2,
     {CmisField::RX_CONTROL_PRE_CURSOR_LANE_45,
      CmisField::RX_CONTROL_POST_CURSOR_LANE_45,
      CmisField::RX_CONTROL_MAIN_LANE_45}},
    {3,
     {CmisField::RX_CONTROL_PRE_CURSOR_LANE_67,
      CmisField::RX_CONTROL_POST_CURSOR_LANE_67,
      CmisField::RX_CONTROL_MAIN_LANE_67}},
};

void getQsfpFieldAddress(
    CmisField field,
    int& dataAddress,
    int& offset,
    int& length) {
  auto info = QsfpFieldInfo<CmisField, CmisPages>::getQsfpFieldAddress(
      cmisFields, field);
  dataAddress = info.dataAddress;
  offset = info.offset;
  length = info.length;
}

bool isValidVdmConfigType(int vdmConf) {
  if (vdmConf == static_cast<int>(SNR_MEDIA_IN) ||
      vdmConf == static_cast<int>(SNR_HOST_IN) ||
      vdmConf == static_cast<int>(PAM4_LTP_MEDIA_IN) ||
      vdmConf == static_cast<int>(PRE_FEC_BER_MEDIA_IN_MIN) ||
      vdmConf == static_cast<int>(PRE_FEC_BER_HOST_IN_MIN) ||
      vdmConf == static_cast<int>(PRE_FEC_BER_MEDIA_IN_MAX) ||
      vdmConf == static_cast<int>(PRE_FEC_BER_HOST_IN_MAX) ||
      vdmConf == static_cast<int>(PRE_FEC_BER_MEDIA_IN_AVG) ||
      vdmConf == static_cast<int>(PRE_FEC_BER_HOST_IN_AVG) ||
      vdmConf == static_cast<int>(PRE_FEC_BER_MEDIA_IN_CUR) ||
      vdmConf == static_cast<int>(PRE_FEC_BER_HOST_IN_CUR) ||
      vdmConf == static_cast<int>(ERR_FRAME_MEDIA_IN_MIN) ||
      vdmConf == static_cast<int>(ERR_FRAME_HOST_IN_MIN) ||
      vdmConf == static_cast<int>(ERR_FRAME_MEDIA_IN_MAX) ||
      vdmConf == static_cast<int>(ERR_FRAME_HOST_IN_MAX) ||
      vdmConf == static_cast<int>(ERR_FRAME_MEDIA_IN_AVG) ||
      vdmConf == static_cast<int>(ERR_FRAME_HOST_IN_AVG) ||
      vdmConf == static_cast<int>(ERR_FRAME_MEDIA_IN_CUR) ||
      vdmConf == static_cast<int>(ERR_FRAME_HOST_IN_CUR) ||
      vdmConf == static_cast<int>(PAM4_LEVEL0_STANDARD_DEVIATION_LINE) ||
      vdmConf == static_cast<int>(PAM4_LEVEL1_STANDARD_DEVIATION_LINE) ||
      vdmConf == static_cast<int>(PAM4_LEVEL2_STANDARD_DEVIATION_LINE) ||
      vdmConf == static_cast<int>(PAM4_LEVEL3_STANDARD_DEVIATION_LINE) ||
      vdmConf == static_cast<int>(PAM4_MPI_LINE) ||
      vdmConf == static_cast<int>(FEC_TAIL_MEDIA_IN_MAX) ||
      vdmConf == static_cast<int>(FEC_TAIL_MEDIA_IN_CURR) ||
      vdmConf == static_cast<int>(FEC_TAIL_HOST_IN_MAX) ||
      vdmConf == static_cast<int>(FEC_TAIL_HOST_IN_CURR)) {
    return true;
  }
  return false;
}

std::optional<CmisModule::ApplicationAdvertisingField>
CmisModule::getApplicationField(uint8_t application, uint8_t startHostLane)
    const {
  for (const auto& capability : moduleCapabilities_) {
    if (capability.moduleMediaInterface == application &&
        std::find(
            capability.hostStartLanes.begin(),
            capability.hostStartLanes.end(),
            startHostLane) != capability.hostStartLanes.end()) {
      return capability;
    }
  }
  return std::nullopt;
}

CmisModule::CmisModule(
    std::set<std::string> portNames,
    TransceiverImpl* qsfpImpl,
    std::shared_ptr<const TransceiverConfig> cfg,
    bool supportRemediate,
    std::string tcvrName)
    : QsfpModule(std::move(portNames), qsfpImpl, std::move(tcvrName)),
      tcvrConfig_(std::move(cfg)),
      supportRemediate_(supportRemediate) {}

CmisModule::~CmisModule() {}

void CmisModule::readCmisField(
    CmisField field,
    uint8_t* data,
    bool skipPageChange) {
  int dataLength, dataPage, dataOffset;
  getQsfpFieldAddress(field, dataPage, dataOffset, dataLength);
  if (static_cast<CmisPages>(dataPage) != CmisPages::LOWER && !flatMem_ &&
      !skipPageChange) {
    // Only change page when it's not a flatMem module (which don't allow
    // changing page) and when the skipPageChange argument is not true
    uint8_t page = static_cast<uint8_t>(dataPage);
    qsfpImpl_->writeTransceiver(
        {TransceiverAccessParameter::ADDR_QSFP,
         127,
         sizeof(page),
         static_cast<int>(CmisPages::LOWER)},
        &page,
        POST_I2C_WRITE_DELAY_US,
        CAST_TO_INT(CmisField::PAGE_CHANGE));
  }
  qsfpImpl_->readTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, dataOffset, dataLength, dataPage},
      data,
      CAST_TO_INT(field));
}

void CmisModule::writeCmisField(
    CmisField field,
    uint8_t* data,
    bool skipPageChange) {
  int dataLength, dataPage, dataOffset;
  getQsfpFieldAddress(field, dataPage, dataOffset, dataLength);
  if (static_cast<CmisPages>(dataPage) != CmisPages::LOWER && !flatMem_ &&
      !skipPageChange) {
    // Only change page when it's not a flatMem module (which don't allow
    // changing page) and when the skipPageChange argument is not true
    uint8_t page = static_cast<uint8_t>(dataPage);
    qsfpImpl_->writeTransceiver(
        {TransceiverAccessParameter::ADDR_QSFP,
         127,
         sizeof(page),
         static_cast<int>(CmisPages::LOWER)},
        &page,
        POST_I2C_WRITE_DELAY_US,
        CAST_TO_INT(CmisField::PAGE_CHANGE));
  }
  qsfpImpl_->writeTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, dataOffset, dataLength, dataPage},
      data,
      POST_I2C_WRITE_DELAY_US,
      CAST_TO_INT(field));
}

FlagLevels CmisModule::getQsfpSensorFlags(CmisField fieldName, int offset) {
  int dataOffset;
  int dataLength;
  int dataAddress;

  getQsfpFieldAddress(fieldName, dataAddress, dataOffset, dataLength);
  const uint8_t* data = getQsfpValuePtr(dataAddress, dataOffset, dataLength);

  // CMIS uses different mappings for flags than Sff therefore not using
  // getQsfpFlags here
  FlagLevels flags;
  CHECK_GE(offset, 0);
  CHECK_LE(offset, 4);
  flags.alarm()->high() = (*data & (1 << offset));
  flags.alarm()->low() = (*data & (1 << ++offset));
  flags.warn()->high() = (*data & (1 << ++offset));
  flags.warn()->low() = (*data & (1 << ++offset));

  return flags;
}

double CmisModule::getQsfpDACLength() const {
  uint8_t value;
  getFieldValueLocked(CmisField::LENGTH_COPPER, &value);
  auto base = value & FieldMasks::CABLE_LENGTH_MASK;
  auto multiplier =
      std::pow(10, value >> 6) * qsfpMultiplier.at(CmisField::LENGTH_COPPER);
  return base * multiplier;
}

double CmisModule::getQsfpSMFLength() const {
  if (flatMem_) {
    return 0;
  }
  uint8_t value;
  getFieldValueLocked(CmisField::LENGTH_SMF, &value);
  auto base = value & FieldMasks::CABLE_LENGTH_MASK;
  auto multiplier =
      std::pow(10, value >> 6) * qsfpMultiplier.at(CmisField::LENGTH_SMF);
  return base * multiplier;
}

double CmisModule::getQsfpOMLength(CmisField field) const {
  if (flatMem_) {
    return 0;
  }
  uint8_t value;
  getFieldValueLocked(field, &value);
  return value * qsfpMultiplier.at(field);
}

GlobalSensors CmisModule::getSensorInfo() {
  GlobalSensors info = GlobalSensors();
  info.temp()->value() =
      getQsfpSensor(CmisField::TEMPERATURE, CmisFieldInfo::getTemp);
  info.temp()->flags() = getQsfpSensorFlags(CmisField::MODULE_ALARMS, 0);
  info.vcc()->value() = getQsfpSensor(CmisField::VCC, CmisFieldInfo::getVcc);
  info.vcc()->flags() = getQsfpSensorFlags(CmisField::MODULE_ALARMS, 4);
  return info;
}

Vendor CmisModule::getVendorInfo() const {
  Vendor vendor = Vendor();
  *vendor.name() = getQsfpString(CmisField::VENDOR_NAME);
  *vendor.oui() = getQsfpString(CmisField::VENDOR_OUI);
  *vendor.partNumber() = getQsfpString(CmisField::PART_NUMBER);
  *vendor.rev() = getQsfpString(CmisField::REVISION_NUMBER);
  *vendor.serialNumber() = getQsfpString(CmisField::VENDOR_SERIAL_NUMBER);
  *vendor.dateCode() = getQsfpString(CmisField::MFG_DATE);
  return vendor;
}

std::array<std::string, 3> CmisModule::getFwRevisions() {
  int offset;
  int length;
  int dataAddress;
  std::array<std::string, 3> fwVersions;
  // Get module f/w version
  getQsfpFieldAddress(
      CmisField::FIRMWARE_REVISION, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);
  fwVersions[0] = fmt::format("{}.{}", data[0], data[1]);
  if (!flatMem_) {
    // Get DSP f/w version
    getQsfpFieldAddress(CmisField::DSP_FW_VERSION, dataAddress, offset, length);
    data = getQsfpValuePtr(dataAddress, offset, length);
    fwVersions[1] = fmt::format("{}.{}", data[0], data[1]);
    // Get the build revision
    getQsfpFieldAddress(CmisField::BUILD_REVISION, dataAddress, offset, length);
    data = getQsfpValuePtr(dataAddress, offset, length);
    fwVersions[2] = fmt::format("{}.{}", data[0], data[1]);
  } else {
    fwVersions[1] = "";
    fwVersions[2] = "";
  }
  return fwVersions;
}

Cable CmisModule::getCableInfo() {
  Cable cable = Cable();
  cable.transmitterTech() = getQsfpTransmitterTechnology();
  cable.mediaTypeEncoding() = getMediaTypeEncoding();

  if (auto length = getQsfpSMFLength(); length != 0) {
    cable.singleMode() = length;
  }
  if (auto length = getQsfpOMLength(CmisField::LENGTH_OM5); length != 0) {
    cable.om5() = length;
  }
  if (auto length = getQsfpOMLength(CmisField::LENGTH_OM4); length != 0) {
    cable.om4() = length;
  }
  if (auto length = getQsfpOMLength(CmisField::LENGTH_OM3); length != 0) {
    cable.om3() = length;
  }
  if (auto length = getQsfpOMLength(CmisField::LENGTH_OM2); length != 0) {
    cable.om2() = length;
  }
  if (auto length = getQsfpDACLength(); length != 0) {
    cable.length() = length;
  }
  return cable;
}

FirmwareStatus CmisModule::getFwStatus() {
  FirmwareStatus fwStatus;
  auto fwRevisions = getFwRevisions();
  fwStatus.version() = fwRevisions[0];
  fwStatus.dspFwVer() = fwRevisions[1];
  fwStatus.buildRev() = fwRevisions[2];
  fwStatus.fwFault() =
      (getSettingsValue(CmisField::MODULE_FLAG, FWFAULT_MASK) >> 1);
  return fwStatus;
}

ModuleStatus CmisModule::getModuleStatus() {
  ModuleStatus moduleStatus;
  moduleStatus.cmisModuleState() =
      (CmisModuleState)(getSettingsValue(CmisField::MODULE_STATE) >> 1);
  moduleStatus.fwStatus() = getFwStatus();
  moduleStatus.cmisStateChanged() = getModuleStateChanged();
  return moduleStatus;
}

/*
 * Threhold values are stored just once;  they aren't per-channel,
 * so in all cases we simple assemble two-byte values and convert
 * them based on the type of the field.
 */
ThresholdLevels CmisModule::getThresholdValues(
    CmisField field,
    double (*conversion)(uint16_t value)) {
  int offset;
  int length;
  int dataAddress;

  CHECK(!flatMem_);

  ThresholdLevels thresh;

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  CHECK_GE(length, 8);
  thresh.alarm()->high() = conversion(data[0] << 8 | data[1]);
  thresh.alarm()->low() = conversion(data[2] << 8 | data[3]);
  thresh.warn()->high() = conversion(data[4] << 8 | data[5]);
  thresh.warn()->low() = conversion(data[6] << 8 | data[7]);

  // For Tx Bias threshold, take care of multiplier
  if (field == CmisField::TX_BIAS_THRESH) {
    getQsfpFieldAddress(
        CmisField::TX_BIAS_MULTIPLIER, dataAddress, offset, length);
    data = getQsfpValuePtr(dataAddress, offset, length);
    auto biasMultiplier = CmisFieldInfo::getTxBiasMultiplier(data[0]);

    thresh.alarm()->high() = thresh.alarm()->high().value() * biasMultiplier;
    thresh.alarm()->low() = thresh.alarm()->low().value() * biasMultiplier;
    thresh.warn()->high() = thresh.warn()->high().value() * biasMultiplier;
    thresh.warn()->low() = thresh.warn()->low().value() * biasMultiplier;
  }

  return thresh;
}

std::optional<AlarmThreshold> CmisModule::getThresholdInfo() {
  if (flatMem_) {
    return {};
  }
  AlarmThreshold threshold = AlarmThreshold();
  threshold.temp() =
      getThresholdValues(CmisField::TEMPERATURE_THRESH, CmisFieldInfo::getTemp);
  threshold.vcc() =
      getThresholdValues(CmisField::VCC_THRESH, CmisFieldInfo::getVcc);
  threshold.rxPwr() =
      getThresholdValues(CmisField::RX_PWR_THRESH, CmisFieldInfo::getPwr);
  threshold.txPwr() =
      getThresholdValues(CmisField::TX_PWR_THRESH, CmisFieldInfo::getPwr);
  threshold.txBias() =
      getThresholdValues(CmisField::TX_BIAS_THRESH, CmisFieldInfo::getTxBias);
  return threshold;
}

uint8_t CmisModule::getSettingsValue(CmisField field, uint8_t mask) const {
  int offset;
  int length;
  int dataAddress;

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  return data[0] & mask;
}

TransceiverSettings CmisModule::getTransceiverSettingsInfo() {
  TransceiverSettings settings = TransceiverSettings();
  if (!flatMem_) {
    settings.cdrTx() = CmisFieldInfo::getFeatureState(
        getSettingsValue(CmisField::TX_SIG_INT_CONT_AD, CDR_IMPL_MASK),
        getSettingsValue(CmisField::TX_CDR_CONTROL));
    settings.cdrRx() = CmisFieldInfo::getFeatureState(
        getSettingsValue(CmisField::RX_SIG_INT_CONT_AD, CDR_IMPL_MASK),
        getSettingsValue(CmisField::RX_CDR_CONTROL));
  } else {
    settings.cdrTx() = FeatureState::UNSUPPORTED;
    settings.cdrRx() = FeatureState::UNSUPPORTED;
  }
  settings.powerMeasurement() =
      flatMem_ ? FeatureState::UNSUPPORTED : FeatureState::ENABLED;

  settings.powerControl() = getPowerControlValue(true /* readFromCache */);
  settings.rateSelect() = flatMem_ ? RateSelectState::UNSUPPORTED
                                   : RateSelectState::APPLICATION_RATE_SELECT;
  settings.rateSelectSetting() = RateSelectSetting::UNSUPPORTED;

  settings.mediaLaneSettings() =
      std::vector<MediaLaneSettings>(numMediaLanes());
  settings.hostLaneSettings() = std::vector<HostLaneSettings>(numHostLanes());

  if (!flatMem_) {
    if (!getMediaLaneSettings(*(settings.mediaLaneSettings()))) {
      settings.mediaLaneSettings()->clear();
      settings.mediaLaneSettings().reset();
    }

    if (!getHostLaneSettings(*(settings.hostLaneSettings()))) {
      settings.hostLaneSettings()->clear();
      settings.hostLaneSettings().reset();
    }
  }

  settings.mediaInterface() = std::vector<MediaInterfaceId>(numMediaLanes());
  if (!getMediaInterfaceId(*(settings.mediaInterface()))) {
    settings.mediaInterface()->clear();
    settings.mediaInterface().reset();
  }

  return settings;
}

bool CmisModule::getMediaLaneSettings(
    std::vector<MediaLaneSettings>& laneSettings) {
  assert(laneSettings.size() == numMediaLanes());

  if (flatMem_) {
    return false;
  }
  auto txDisable = getSettingsValue(CmisField::TX_DISABLE);
  auto txSquelchDisable = getSettingsValue(CmisField::TX_SQUELCH_DISABLE);
  auto txSquelchForce = getSettingsValue(CmisField::TX_FORCE_SQUELCH);

  for (int lane = 0; lane < laneSettings.size(); lane++) {
    auto laneMask = (1 << lane);
    laneSettings[lane].lane() = lane;
    laneSettings[lane].txDisable() = txDisable & laneMask;
    laneSettings[lane].txSquelch() = txSquelchDisable & laneMask;
    laneSettings[lane].txSquelchForce() = txSquelchForce & laneMask;
  }

  return true;
}

bool CmisModule::getHostLaneSettings(
    std::vector<HostLaneSettings>& laneSettings) {
  const uint8_t* dataPre;
  const uint8_t* dataPost;
  const uint8_t* dataMain;
  int offset;
  int length;
  int dataAddress;

  assert(laneSettings.size() == numHostLanes());

  if (flatMem_) {
    return false;
  }
  auto rxOutput = getSettingsValue(CmisField::RX_DISABLE);
  auto rxSquelchDisable = getSettingsValue(CmisField::RX_SQUELCH_DISABLE);

  getQsfpFieldAddress(
      CmisField::RX_OUT_PRE_CURSOR, dataAddress, offset, length);
  dataPre = getQsfpValuePtr(dataAddress, offset, length);

  getQsfpFieldAddress(
      CmisField::RX_OUT_POST_CURSOR, dataAddress, offset, length);
  dataPost = getQsfpValuePtr(dataAddress, offset, length);

  getQsfpFieldAddress(CmisField::RX_OUT_MAIN, dataAddress, offset, length);
  dataMain = getQsfpValuePtr(dataAddress, offset, length);

  for (int lane = 0; lane < laneSettings.size(); lane++) {
    auto laneMask = (1 << lane);
    laneSettings[lane].lane() = lane;
    laneSettings[lane].rxOutput() = rxOutput & laneMask;
    laneSettings[lane].rxSquelch() = rxSquelchDisable & laneMask;

    uint8_t pre = (dataPre[lane / 2] >> ((lane % 2) * 4)) & 0x0f;
    QSFP_LOG(DBG3, this) << folly::sformat(
        "Lane = {:d}, Pre = {:d}", lane, pre);
    laneSettings[lane].rxOutputPreCursor() = pre;

    uint8_t post = (dataPost[lane / 2] >> ((lane % 2) * 4)) & 0x0f;
    QSFP_LOG(DBG3, this) << folly::sformat(
        "Lane = {:d}, Post = {:d}", lane, post);
    laneSettings[lane].rxOutputPostCursor() = post;

    uint8_t mainVal = (dataMain[lane / 2] >> ((lane % 2) * 4)) & 0x0f;
    QSFP_LOG(DBG3, this) << folly::sformat(
        "Lane = {:d}, Main = {:d}", lane, mainVal);
    laneSettings[lane].rxOutputAmplitude() = mainVal;
  }
  return true;
}

// Returns the currently configured mediaInterfaceCode on a host lane
uint8_t CmisModule::currentConfiguredMediaInterfaceCode(
    uint8_t hostLane) const {
  auto mediaTypeEncoding = getMediaTypeEncoding();
  uint8_t application = 0;
  if (mediaTypeEncoding == MediaTypeEncodings::OPTICAL_SMF) {
    application = getCurrentApplication(hostLane, kMediaInterfaceCodeOffset);
  } else if (
      mediaTypeEncoding == MediaTypeEncodings::PASSIVE_CU &&
      !moduleCapabilities_.empty()) {
    // For Passive DAC cables that don't get programmed, just return the media
    // interface code for the first capability.
    auto firstModuleCapability = moduleCapabilities_.begin();
    application = firstModuleCapability->moduleMediaInterface;
  } else if (mediaTypeEncoding == MediaTypeEncodings::ACTIVE_CABLES) {
    application = getCurrentApplication(hostLane, kHostInterfaceCodeOffset);
  }
  return application;
}

// Returns the list of host lanes configured in the same datapath as the
// provided startHostLane
std::vector<uint8_t> CmisModule::configuredHostLanes(
    uint8_t startHostLane) const {
  std::vector<uint8_t> cfgLanes;
  auto currentMediaInterface =
      currentConfiguredMediaInterfaceCode(startHostLane);
  if (auto applicationAdvertisingField =
          getApplicationField(currentMediaInterface, startHostLane)) {
    for (uint8_t lane = startHostLane;
         lane < startHostLane + applicationAdvertisingField->hostLaneCount;
         lane++) {
      cfgLanes.push_back(lane);
    }
  }
  return cfgLanes;
}

// Returns the list of media lanes configured in the same datapath as the
// provided startHostLane
std::vector<uint8_t> CmisModule::configuredMediaLanes(
    uint8_t startHostLane) const {
  std::vector<uint8_t> cfgLanes;
  if (flatMem_) {
    // FlatMem_ modules won't have page01 to read the media lane assignment
    return cfgLanes;
  }

  auto currentMediaInterface =
      currentConfiguredMediaInterfaceCode(startHostLane);
  if (auto applicationAdvertisingField =
          getApplicationField(currentMediaInterface, startHostLane)) {
    // The assignment byte has a '1' for every datapath that starts at that
    // lane. We first need to find out the 'index (say n)' of the datapath
    // using the given start host lane. We'll then look for a nth '1' in the
    // corresponding media lane assignment.
    // For example, if the hostLaneAssignment is 0x55, the corresponding
    // mediaLaneAssignment can be 0xF. Which means that the pairing of
    // host->media lanes will be (hostLane:0, mediaLane:0), (hostLane:2,
    // mediaLane:1), (hostLane:4, mediaLane:2), (hostLane:6, mediaLane:3)
    auto it = std::find(
        applicationAdvertisingField->hostStartLanes.begin(),
        applicationAdvertisingField->hostStartLanes.end(),
        startHostLane);
    if (it == applicationAdvertisingField->hostStartLanes.end()) {
      QSFP_LOG(ERR, this) << "Couldn't find the hostStartLane "
                          << startHostLane;
      return cfgLanes;
    }

    auto index =
        std::distance(applicationAdvertisingField->hostStartLanes.begin(), it);
    uint8_t mediaStartLane = 0;
    if (index < applicationAdvertisingField->mediaStartLanes.size()) {
      mediaStartLane = applicationAdvertisingField->mediaStartLanes[index];
    } else {
      QSFP_LOG(ERR, this) << "Index " << index << " out of range for "
                          << folly::join(
                                 ",",
                                 applicationAdvertisingField->mediaStartLanes);
      return cfgLanes;
    }

    for (uint8_t start = mediaStartLane;
         start < mediaStartLane + applicationAdvertisingField->mediaLaneCount;
         start++) {
      cfgLanes.push_back(start);
    }
  }
  return cfgLanes;
}

uint8_t CmisModule::getCurrentApplication(uint8_t lane, int byteOffset) const {
  if (lane >= 8) {
    QSFP_LOG(ERR, this) << "Invalid lane number " << lane;
    // Based on SFF-8024, an App / App Sel of 0 is undefined/Unknown.
    return 0;
  }
  // Pick the first application for flatMem modules. FlatMem modules don't
  // support page11h that contains the current operational app sel code
  uint8_t currentApplicationSel = flatMem_
      ? 1
      : getSettingsValue(laneToActiveCtrlField[lane], APP_SEL_MASK);
  // The application sel code is at the higher four bits of the field.
  currentApplicationSel = currentApplicationSel >> APP_SEL_BITSHIFT;

  // Application select value 0 means application is not selected by module yet
  if (currentApplicationSel == 0) {
    QSFP_LOG(ERR, this) << "Module has not selected application yet";
    // Based on SFF-8024, an App / App Sel of 0 is undefined/Unknown.
    return 0;
  }

  uint8_t currentApplication;
  int offset;
  int length;
  int dataAddress;

  // The ApSel value from 1 to 8 are present in the lower page and values from
  // 9 to 15 are in page 1
  if (currentApplicationSel <= 8) {
    getQsfpFieldAddress(
        CmisField::APPLICATION_ADVERTISING1, dataAddress, offset, length);
    // We use the module Media Interface ID for Optical modules, which is
    // located at the second byte of the field (byteOffset = 1), as Application
    // ID here. If we have an AEC cable, we use the module host interface ID
    // which has a byteOffset of 0. This is in page 00h app sel advertising
    // (starting at offset 86 for page)
    offset += (currentApplicationSel - 1) * length + byteOffset;
  } else {
    getQsfpFieldAddress(
        CmisField::APPLICATION_ADVERTISING2, dataAddress, offset, length);
    // This page contains Module media interface id on second byte of field
    // for ApSel 9 onwards
    offset += (currentApplicationSel - 9) * length + byteOffset;
  }

  getQsfpValue(dataAddress, offset, 1, &currentApplication);

  return currentApplication;
}

MediaTypeEncodings CmisModule::getMediaTypeEncoding() const {
  return static_cast<MediaTypeEncodings>(
      getSettingsValue(CmisField::MEDIA_TYPE_ENCODINGS));
}

bool CmisModule::getMediaInterfaceId(
    std::vector<MediaInterfaceId>& mediaInterface) {
  assert(mediaInterface.size() == numMediaLanes());
  MediaTypeEncodings encoding = getMediaTypeEncoding();
  if (encoding == MediaTypeEncodings::OPTICAL_SMF) {
    for (int lane = 0; lane < mediaInterface.size(); lane++) {
      auto smfMediaInterface = getSmfMediaInterface(lane);
      mediaInterface[lane].lane() = lane;
      MediaInterfaceUnion media;
      media.smfCode() = smfMediaInterface;
      mediaInterface[lane].code() =
          CmisHelper::getMediaInterfaceCode<SMFMediaInterfaceCode>(
              smfMediaInterface, CmisHelper::getSmfMediaInterfaceMapping());
      if (mediaInterface[lane].code() == MediaInterfaceCode::UNKNOWN) {
        QSFP_LOG(ERR, this)
            << "Unable to find MediaInterfaceCode for "
            << apache::thrift::util::enumNameSafe(smfMediaInterface);
      }
      mediaInterface[lane].media() = media;
    }
  } else if (
      encoding == MediaTypeEncodings::PASSIVE_CU &&
      !moduleCapabilities_.empty()) {
    // For Passive DAC cables that don't get programmed, just return the media
    // interface code for the first capability.
    auto firstModuleCapability = moduleCapabilities_.begin();
    for (int lane = 0; lane < mediaInterface.size(); lane++) {
      mediaInterface[lane].lane() = lane;
      MediaInterfaceUnion media;
      media.passiveCuCode() = static_cast<PassiveCuMediaInterfaceCode>(
          firstModuleCapability->moduleMediaInterface);
      // FIXME: Remove CR8_400G hardcoding and derive this from number of
      // lanes/host electrical interface instead
      mediaInterface[lane].code() = MediaInterfaceCode::CR8_400G;
      mediaInterface[lane].media() = media;
    }
  } else if (encoding == MediaTypeEncodings::ACTIVE_CABLES) {
    for (int lane = 0; lane < mediaInterface.size(); lane++) {
      auto activeCuInterfaceCode = getActiveCuMediaInterface(lane);
      mediaInterface[lane].lane() = lane;
      MediaInterfaceUnion media;
      media.activeCuCode() = activeCuInterfaceCode;
      mediaInterface[lane].code() =
          CmisHelper::getMediaInterfaceCode<ActiveCuHostInterfaceCode>(
              activeCuInterfaceCode,
              CmisHelper::getActiveMediaInterfaceMapping());
      if (mediaInterface[lane].code() == MediaInterfaceCode::UNKNOWN) {
        QSFP_LOG(ERR, this)
            << "Unable to find MediaInterfaceCode for "
            << apache::thrift::util::enumNameSafe(activeCuInterfaceCode);
      }
      mediaInterface[lane].media() = media;
    }
  } else {
    return false;
  }

  return true;
}

void CmisModule::getApplicationCapabilities() {
  const uint8_t* data;
  int offset;
  int length;
  int dataAddress;

  moduleCapabilities_.clear();
  for (uint8_t i = 0; i < 8; i++) {
    getQsfpFieldAddress(
        CmisField::APPLICATION_ADVERTISING1, dataAddress, offset, length);
    data = getQsfpValuePtr(dataAddress, offset + i * length, length);

    if (data[0] == 0xff) {
      break;
    }

    QSFP_LOG(DBG3, this) << folly::sformat(
        "Adding module capability: {:#x} at position {:d}", data[1], i + 1);
    ApplicationAdvertisingField applicationAdvertisingField;
    applicationAdvertisingField.ApSelCode = (i + 1);
    // For Active cables, we use the Host Interface Code as the designated
    // identifier for the rate of the application. The Media side of active
    // cables, per spec, specifies only the BER for the cable, which might
    // be the same for all the supported rates.
    if (isAecModule()) {
      applicationAdvertisingField.moduleMediaInterface = data[0];
    } else {
      applicationAdvertisingField.moduleMediaInterface = data[1];
    }
    applicationAdvertisingField.moduleHostInterface = data[0];
    applicationAdvertisingField.hostLaneCount =
        (data[2] & FieldMasks::UPPER_FOUR_BITS_MASK) >> 4;
    applicationAdvertisingField.mediaLaneCount =
        data[2] & FieldMasks::LOWER_FOUR_BITS_MASK;
    for (int lane = 0; lane < 8; lane++) {
      if (data[3] & (1 << lane)) {
        applicationAdvertisingField.hostStartLanes.push_back(lane);
      }
    }

    if (!flatMem_) {
      getQsfpFieldAddress(
          CmisField::MEDIA_LANE_ASSIGNMENT, dataAddress, offset, length);
      offset += i;
      uint8_t mediaLaneAssignment;
      getQsfpValue(dataAddress, offset, 1, &mediaLaneAssignment);
      for (int lane = 0; lane < 8; lane++) {
        if (mediaLaneAssignment & (1 << lane)) {
          applicationAdvertisingField.mediaStartLanes.push_back(lane);
        }
      }
    }

    moduleCapabilities_.push_back(applicationAdvertisingField);
  }
}

PowerControlState CmisModule::getPowerControlValue(bool readFromCache) {
  uint8_t moduleControl;
  if (readFromCache) {
    moduleControl = getSettingsValue(
        CmisField::MODULE_CONTROL, uint8_t(POWER_CONTROL_MASK));
  } else {
    readCmisField(CmisField::MODULE_CONTROL, &moduleControl);
    moduleControl &= POWER_CONTROL_MASK;
  }
  if (moduleControl) {
    return PowerControlState::POWER_LPMODE;
  } else {
    return PowerControlState::HIGH_POWER_OVERRIDE;
  }
}

PowerControlState CmisModule::getCurrentPowerControlState() {
  uint8_t currentModuleControl;
  readCmisField(CmisField::MODULE_CONTROL, &currentModuleControl);

  if (currentModuleControl & POWER_CONTROL_MASK) {
    QSFP_LOG(INFO, this)
        << "getCurrentPowerControlState: Current power state is LOW POWER MODE (LP bit set)";
    return PowerControlState::POWER_LPMODE;
  } else {
    QSFP_LOG(INFO, this)
        << "getCurrentPowerControlState: Current power state is HIGH POWER MODE";
    return PowerControlState::HIGH_POWER_OVERRIDE;
  }
}

bool CmisModule::isModuleInReadyState() {
  uint8_t moduleStatus;
  readCmisField(CmisField::MODULE_STATE, &moduleStatus);
  const bool isReady =
      ((CmisModuleState)((moduleStatus & MODULE_STATUS_MASK) >>
                         MODULE_STATUS_BITSHIFT) == CmisModuleState::READY);

  if (isReady) {
    QSFP_LOG(INFO, this) << "isModuleInReadyState: Module is in READY state";
  } else {
    QSFP_LOG(INFO, this)
        << "isModuleInReadyState: Module is not ready yet - need more time to be ready";
  }

  return isReady;
}

bool CmisModule::moduleReadyStatePoll() {
  auto retries = 0;
  constexpr int kUsecModuleReadyStateUpdateTimeMax = 5000000; // 5 seconds
  constexpr int kUsecModuleReadyStatePollTime = 100000; // 100 ms
  auto maxRetriesReady =
      kUsecModuleReadyStateUpdateTimeMax / kUsecModuleReadyStatePollTime;

  while (retries++ < maxRetriesReady) {
    /* sleep override */
    usleep(kUsecModuleReadyStatePollTime);
    if (isModuleInReadyState()) {
      return true;
    }
  }
  if (retries >= maxRetriesReady) {
    QSFP_LOG(ERR, this) << folly::sformat(
        "Module not ready even after waiting {:d} uSec",
        kUsecModuleReadyStateUpdateTimeMax);
  }
  return false;
}

/*
 * For the specified field, collect alarm and warning flags for the channel.
 */

FlagLevels CmisModule::getChannelFlags(CmisField field, int channel) {
  FlagLevels flags;
  int offset;
  int length;
  int dataAddress;

  CHECK_GE(channel, 0);
  if (channel > 8) {
    QSFP_LOG(ERR, this) << folly::sformat(
        "getChannelFlags: Channel id {:d} is invalid", channel);
    return FlagLevels{};
  }

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  flags.warn()->low() = (data[3] & (1 << channel));
  flags.warn()->high() = (data[2] & (1 << channel));
  flags.alarm()->low() = (data[1] & (1 << channel));
  flags.alarm()->high() = (data[0] & (1 << channel));

  return flags;
}

/*
 * Iterate through channels collecting appropriate data;
 */

bool CmisModule::getSignalsPerMediaLane(
    std::vector<MediaLaneSignals>& signals) {
  assert(signals.size() == numMediaLanes());
  if (flatMem_) {
    return false;
  }

  auto rxLos = getSettingsValue(CmisField::RX_LOS_FLAG);
  auto rxLol = getSettingsValue(CmisField::RX_LOL_FLAG);
  auto txFault = getSettingsValue(CmisField::TX_FAULT_FLAG);

  for (int lane = 0; lane < signals.size(); lane++) {
    auto laneMask = (1 << lane);
    signals[lane].lane() = lane;
    signals[lane].rxLos() = rxLos & laneMask;
    signals[lane].rxLol() = rxLol & laneMask;
    signals[lane].txFault() = txFault & laneMask;
  }

  return true;
}

/*
 * Iterate through channels collecting appropriate data;
 */

bool CmisModule::getSignalsPerHostLane(std::vector<HostLaneSignals>& signals) {
  assert(signals.size() == numHostLanes());
  if (flatMem_) {
    return false;
  }

  auto dataPathDeInit = getSettingsValue(CmisField::DATA_PATH_DEINIT);

  auto txLos = getSettingsValue(CmisField::TX_LOS_FLAG);
  auto txLol = getSettingsValue(CmisField::TX_LOL_FLAG);
  auto txEq = getSettingsValue(CmisField::TX_EQ_FLAG);

  for (int lane = 0; lane < signals.size(); lane++) {
    signals[lane].lane() = lane;
    signals[lane].dataPathDeInit() = dataPathDeInit & (1 << lane);

    signals[lane].cmisLaneState() = getDatapathLaneStateLocked(lane);

    auto laneMask = (1 << lane);
    signals[lane].lane() = lane;
    signals[lane].txLos() = txLos & laneMask;
    signals[lane].txLol() = txLol & laneMask;
    signals[lane].txAdaptEqFault() = txEq & laneMask;
  }

  return true;
}

/*
 * Iterate through channels collecting appropriate data;
 */

bool CmisModule::getSensorsPerChanInfo(std::vector<Channel>& channels) {
  if (flatMem_) {
    return false;
  }
  const uint8_t* data;
  int offset;
  int length;
  int dataAddress;

  for (int channel = 0; channel < numMediaLanes(); channel++) {
    channels[channel].sensors()->rxPwr()->flags() =
        getChannelFlags(CmisField::RX_PWR_FLAG, channel);
  }

  for (int channel = 0; channel < numMediaLanes(); channel++) {
    channels[channel].sensors()->txBias()->flags() =
        getChannelFlags(CmisField::TX_BIAS_FLAG, channel);
  }

  for (int channel = 0; channel < numMediaLanes(); channel++) {
    channels[channel].sensors()->txPwr()->flags() =
        getChannelFlags(CmisField::TX_PWR_FLAG, channel);
  }

  getQsfpFieldAddress(CmisField::CHANNEL_RX_PWR, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    auto pwr = CmisFieldInfo::getPwr(value); // This is in mW
    channel.sensors()->rxPwr()->value() = pwr;
    Sensor rxDbm;
    rxDbm.value() = mwToDb(pwr);
    channel.sensors()->rxPwrdBm() = rxDbm;
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  // For Tx bias, take care of multiplier
  getQsfpFieldAddress(
      CmisField::TX_BIAS_MULTIPLIER, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);
  auto biasMultiplier = CmisFieldInfo::getTxBiasMultiplier(data[0]);

  getQsfpFieldAddress(CmisField::CHANNEL_TX_BIAS, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);
  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    channel.sensors()->txBias()->value() =
        CmisFieldInfo::getTxBias(value) * biasMultiplier;
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  getQsfpFieldAddress(CmisField::CHANNEL_TX_PWR, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (auto& channel : channels) {
    uint16_t value = data[0] << 8 | data[1];
    auto pwr = CmisFieldInfo::getPwr(value); // This is in mW
    channel.sensors()->txPwr()->value() = pwr;
    Sensor txDbm;
    txDbm.value() = mwToDb(pwr);
    channel.sensors()->txPwrdBm() = txDbm;
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  getQsfpFieldAddress(
      CmisField::MEDIA_BER_HOST_SNR, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (auto& channel : channels) {
    // SNR value are LSB.
    uint16_t value = data[1] << 8 | data[0];
    channel.sensors()->txSnr() = Sensor();
    channel.sensors()->txSnr()->value() = CmisFieldInfo::getSnr(value);
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  getQsfpFieldAddress(CmisField::MEDIA_SNR, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (auto& channel : channels) {
    // SNR value are LSB.
    uint16_t value = data[1] << 8 | data[0];
    channel.sensors()->rxSnr() = Sensor();

    // Compute SNR value from raw value
    double snrValue = CmisFieldInfo::getSnr(value);

    // Apply Rx-SNR correction (returns corrected value)
    snrValue = applyRxSnrCorrection(value, snrValue);

    channel.sensors()->rxSnr()->value() = snrValue;
    data += 2;
    length--;
  }
  CHECK_GE(length, 0);

  return true;
}

std::string CmisModule::getQsfpString(CmisField field) const {
  int offset;
  int length;
  int dataAddress;

  getQsfpFieldAddress(field, dataAddress, offset, length);
  const uint8_t* data = getQsfpValuePtr(dataAddress, offset, length);

  while (length > 0 && data[length - 1] == ' ') {
    --length;
  }

  std::string value(reinterpret_cast<const char*>(data), length);
  return validateQsfpString(value) ? value : "UNKNOWN";
}

double CmisModule::getQsfpSensor(
    CmisField field,
    double (*conversion)(uint16_t value)) {
  auto info = QsfpFieldInfo<CmisField, CmisPages>::getQsfpFieldAddress(
      cmisFields, field);
  const uint8_t* data =
      getQsfpValuePtr(info.dataAddress, info.offset, info.length);
  return conversion(data[0] << 8 | data[1]);
}

TransmitterTechnology CmisModule::getQsfpTransmitterTechnology() const {
  auto info = QsfpFieldInfo<CmisField, CmisPages>::getQsfpFieldAddress(
      cmisFields, CmisField::MEDIA_INTERFACE_TECHNOLOGY);
  const uint8_t* data =
      getQsfpValuePtr(info.dataAddress, info.offset, info.length);

  uint8_t transTech = *data;
  if (transTech == DeviceTechnologyCmis::UNKNOWN_VALUE_CMIS) {
    return TransmitterTechnology::UNKNOWN;
    // TODO(T232092663): Fix this, introduce the TUNABLE_OPTICS
  } else if (
      (transTech <= DeviceTechnologyCmis::OPTICAL_MAX_VALUE_CMIS) ||
      (transTech == DeviceTechnologyCmis::C_BAND_TUNABLE_LASER_CMIS) ||
      (transTech == DeviceTechnologyCmis::L_BAND_TUNABLE_LASER_CMIS)) {
    return TransmitterTechnology::OPTICAL;
  } else {
    return TransmitterTechnology::COPPER;
  }
}

SignalFlags CmisModule::getSignalFlagInfo() {
  SignalFlags signalFlags = SignalFlags();

  if (!flatMem_) {
    signalFlags.txLos() = getSettingsValue(CmisField::TX_LOS_FLAG);
    signalFlags.rxLos() = getSettingsValue(CmisField::RX_LOS_FLAG);
    signalFlags.txLol() = getSettingsValue(CmisField::TX_LOL_FLAG);
    signalFlags.rxLol() = getSettingsValue(CmisField::RX_LOL_FLAG);
  }
  return signalFlags;
}

/*
 * updateVdmDiagsValLocation
 *
 * This function scans the VDM config pages by looking into each 2 byte
 * descriptors. It builds up the mapping from VDM config type to the VDM value
 * location (page, offset and length). These config could be module based config
 * or lane/datapath based config. The function updates the lowest offset of the
 * corresponding VDM data value. For config present in VDM page 0x20-23, the
 * corresponding data is present in VDM pages 0x24-27
 */
void CmisModule::updateVdmDiagsValLocation() {
  if (!cacheIsValid() || !isVdmSupported()) {
    QSFP_LOG(DBG2, this) << "Module does not support VDM diagnostics";
    return;
  }

  // The VdmConf can be present at any offset from page 0x20 to 0x22. Check all
  // the descriptors (2 bytes) on these pages
  std::vector<CmisField> cmisVdmConfPages = {
      CmisField::PAGE_UPPER20H, CmisField::PAGE_UPPER21H};
  if (isVdmSupported(3)) {
    cmisVdmConfPages.push_back(CmisField::PAGE_UPPER22H);
  }

  for (auto field : cmisVdmConfPages) {
    int page;
    int startOffset;
    int endOffset;
    int length;
    uint8_t data[128];
    const uint8_t* dataPtr = data;
    getQsfpFieldAddress(field, page, startOffset, length);
    endOffset = startOffset + length - 1;
    readFromCacheOrHw(field, data, true);

    // Each 2 byte descriptor:
    //    byte_1[7..0] -> VDM config type
    enum VdmConfigType lastConfig = UNSUPPORTED;
    for (auto offset = startOffset; offset <= endOffset;
         offset += kVdmDescriptorLength, dataPtr += kVdmDescriptorLength) {
      if (isValidVdmConfigType(dataPtr[1])) {
        if (static_cast<VdmConfigType>(dataPtr[1]) == lastConfig) {
          vdmConfigDataLocations_[lastConfig].vdmValLength += 2;
        } else {
          VdmDiagsLocationStatus vdmConfStatus;
          vdmConfStatus.vdmConfImplementedByModule = true;
          vdmConfStatus.vdmValPage = static_cast<CmisPages>(page + 4);
          vdmConfStatus.vdmValOffset = offset;
          vdmConfStatus.vdmValLength = 2;
          // Extract bits 7-4 from byte 0 (Even Address byte) for
          // LocalThresholdSetID
          vdmConfStatus.localThresholdSetID = (dataPtr[0] >> 4) & 0x0F;
          lastConfig = static_cast<VdmConfigType>(dataPtr[1]);
          vdmConfigDataLocations_[lastConfig] = vdmConfStatus;
        }
      }
    }
  }

  QSFP_LOG(DBG2, this) << "Module's VDM Config Locations found:";
  for (auto& it : vdmConfigDataLocations_) {
    QSFP_LOG(DBG2, this) << "VDM Config Type: " << static_cast<int>(it.first)
                         << ", Page: " << static_cast<int>(it.second.vdmValPage)
                         << ", Offset: " << it.second.vdmValOffset
                         << ", Length: " << it.second.vdmValLength;
  }
}

/*
 * getVdmDiagsValLocation
 *
 * For a given VDM config type, this function returns the VDM data location
 * values.
 */
CmisModule::VdmDiagsLocationStatus CmisModule::getVdmDiagsValLocation(
    VdmConfigType vdmConf) const {
  // Try to return VDM data location info now. If still no info available then
  // return empty values
  if (vdmConfigDataLocations_.find(vdmConf) == vdmConfigDataLocations_.end()) {
    return CmisModule::VdmDiagsLocationStatus{};
  }
  return vdmConfigDataLocations_.at(vdmConf);
}

/*
 * getCdbSymbolErrorHistogramLocked
 *
 * Return symbol error histogram data for all bins for a given datapath id and
 * the media/host side
 */
std::map<int32_t, SymErrHistogramBin>
CmisModule::getCdbSymbolErrorHistogramLocked(
    uint8_t datapathId,
    bool mediaSide) {
  std::map<int32_t, SymErrHistogramBin> histData;
  CdbCommandBlock commandBlockBuf;

  commandBlockBuf.createCdbCmdSymbolErrorHistogram(datapathId, mediaSide);
  auto ret = commandBlockBuf.cmisRunCdbCommand(qsfpImpl_);
  if (ret && commandBlockBuf.getCdbRlplLength() >= 1) {
    int numBins = commandBlockBuf.getCdbLplFlatMemory()[0];
    for (auto bin = 0; bin < numBins; bin++) {
      SymErrHistogramBin binHistData;
      binHistData.nbitSymbolErrorMax() = f16ToDouble(
          commandBlockBuf.getCdbLplFlatMemory()
              [bin * kCdbSymErrHistBinSize + kCdbSymErrHistMaxOffset],
          commandBlockBuf.getCdbLplFlatMemory()
              [bin * kCdbSymErrHistBinSize + kCdbSymErrHistMaxOffset + 1]);
      binHistData.nbitSymbolErrorAvg() = f16ToDouble(
          commandBlockBuf.getCdbLplFlatMemory()
              [bin * kCdbSymErrHistBinSize + kCdbSymErrHistAvgOffset],
          commandBlockBuf.getCdbLplFlatMemory()
              [bin * kCdbSymErrHistBinSize + kCdbSymErrHistAvgOffset + 1]);
      binHistData.nbitSymbolErrorCur() = f16ToDouble(
          commandBlockBuf.getCdbLplFlatMemory()
              [bin * kCdbSymErrHistBinSize + kCdbSymErrHistCurOffset],
          commandBlockBuf.getCdbLplFlatMemory()
              [bin * kCdbSymErrHistBinSize + kCdbSymErrHistCurOffset + 1]);
      histData[bin] = binHistData;
    }
  }
  return histData;
}

/*
 * getCdbSymbolErrorHistogramLocked
 *
 * Return symbol error histogram data for all bins for all datapaths for the
 * both media/host side
 */
std::map<std::string, CdbDatapathSymErrHistogram>
CmisModule::getCdbSymbolErrorHistogramLocked() {
  std::map<std::string, CdbDatapathSymErrHistogram> cdbDpSymErrHist;
  for (auto& [portName, hostLanes] : getPortNameToHostLanes()) {
    // Datapath Id is same as first lane Id
    int datapathId = *hostLanes.begin();
    cdbDpSymErrHist[portName].media() =
        getCdbSymbolErrorHistogramLocked(datapathId, true);
    cdbDpSymErrHist[portName].host() =
        getCdbSymbolErrorHistogramLocked(datapathId, false);
  }
  return cdbDpSymErrHist;
}

std::optional<VdmDiagsStats> CmisModule::getVdmDiagsStatsInfo() {
  VdmDiagsStats vdmStats;
  const uint8_t* data;
  int offset;
  int length;
  int dataAddress;

  if (!isVdmSupported() || !cacheIsValid()) {
    return std::nullopt;
  }

  vdmStats.statsCollectionTme() = WallClockUtil::NowInSecFast();

  // Fill in channel SNR Media In
  auto vdmDiagsValLocation = getVdmDiagsValLocation(SNR_MEDIA_IN);
  if (vdmDiagsValLocation.vdmConfImplementedByModule) {
    dataAddress = static_cast<int>(vdmDiagsValLocation.vdmValPage);
    offset = vdmDiagsValLocation.vdmValOffset;
    length = vdmDiagsValLocation.vdmValLength;
    data = getQsfpValuePtr(dataAddress, offset, length);
    for (auto lanes = 0; lanes < length / 2; lanes++) {
      double snr;
      snr = data[lanes * 2] + (data[lanes * 2 + 1] / kU16TypeLsbDivisor);
      vdmStats.eSnrMediaChannel()[lanes] = snr;
    }
  }

  // Lambda to extract BER or Frame Error values for a given VDM config type
  auto captureVdmBerFrameErrorValues =
      [&](VdmConfigType vdmConfType) -> std::optional<double> {
    vdmDiagsValLocation = getVdmDiagsValLocation(vdmConfType);
    if (vdmDiagsValLocation.vdmConfImplementedByModule) {
      dataAddress = static_cast<int>(vdmDiagsValLocation.vdmValPage);
      offset = vdmDiagsValLocation.vdmValOffset;
      length = vdmDiagsValLocation.vdmValLength;
      data = getQsfpValuePtr(dataAddress, offset, length);
      return f16ToDouble(data[0], data[1]);
    }
    return std::nullopt;
  };

  // Fill in Media Pre FEC BER values
  if (auto berVal = captureVdmBerFrameErrorValues(PRE_FEC_BER_MEDIA_IN_MIN)) {
    vdmStats.preFecBerMediaMin() = berVal.value();
  }

  if (auto berVal = captureVdmBerFrameErrorValues(PRE_FEC_BER_MEDIA_IN_MAX)) {
    vdmStats.preFecBerMediaMax() = berVal.value();
  }

  if (auto berVal = captureVdmBerFrameErrorValues(PRE_FEC_BER_MEDIA_IN_AVG)) {
    vdmStats.preFecBerMediaAvg() = berVal.value();
  }

  if (auto berVal = captureVdmBerFrameErrorValues(PRE_FEC_BER_MEDIA_IN_CUR)) {
    vdmStats.preFecBerMediaCur() = berVal.value();
  }

  if (auto fecTailMax = captureVdmBerFrameErrorValues(FEC_TAIL_MEDIA_IN_MAX)) {
    vdmStats.fecTailMediaMax() = fecTailMax.value();
  }

  if (auto fecTailCurr =
          captureVdmBerFrameErrorValues(FEC_TAIL_MEDIA_IN_CURR)) {
    vdmStats.fecTailMediaCurr() = fecTailCurr.value();
  }

  // Fill in Host Pre FEC BER values
  if (auto berVal = captureVdmBerFrameErrorValues(PRE_FEC_BER_HOST_IN_MIN)) {
    vdmStats.preFecBerHostMin() = berVal.value();
  }

  if (auto berVal = captureVdmBerFrameErrorValues(PRE_FEC_BER_HOST_IN_MAX)) {
    vdmStats.preFecBerHostMax() = berVal.value();
  }

  if (auto berVal = captureVdmBerFrameErrorValues(PRE_FEC_BER_HOST_IN_AVG)) {
    vdmStats.preFecBerHostAvg() = berVal.value();
  }

  if (auto berVal = captureVdmBerFrameErrorValues(PRE_FEC_BER_HOST_IN_CUR)) {
    vdmStats.preFecBerHostCur() = berVal.value();
  }

  // Fill in Media Post FEC Errored Frames values
  if (auto errFrames = captureVdmBerFrameErrorValues(ERR_FRAME_MEDIA_IN_MIN)) {
    vdmStats.errFrameMediaMin() = errFrames.value();
  }

  if (auto errFrames = captureVdmBerFrameErrorValues(ERR_FRAME_MEDIA_IN_MAX)) {
    vdmStats.errFrameMediaMax() = errFrames.value();
  }

  if (auto errFrames = captureVdmBerFrameErrorValues(ERR_FRAME_MEDIA_IN_AVG)) {
    vdmStats.errFrameMediaAvg() = errFrames.value();
  }

  if (auto errFrames = captureVdmBerFrameErrorValues(ERR_FRAME_MEDIA_IN_CUR)) {
    vdmStats.errFrameMediaCur() = errFrames.value();
  }

  // Fill in Host Post FEC Errored Frame values
  if (auto errFrames = captureVdmBerFrameErrorValues(ERR_FRAME_HOST_IN_MIN)) {
    vdmStats.errFrameHostMin() = errFrames.value();
  }

  if (auto errFrames = captureVdmBerFrameErrorValues(ERR_FRAME_HOST_IN_MAX)) {
    vdmStats.errFrameHostMax() = errFrames.value();
  }

  if (auto errFrames = captureVdmBerFrameErrorValues(ERR_FRAME_HOST_IN_AVG)) {
    vdmStats.errFrameHostAvg() = errFrames.value();
  }

  if (auto errFrames = captureVdmBerFrameErrorValues(ERR_FRAME_HOST_IN_CUR)) {
    vdmStats.errFrameHostCur() = errFrames.value();
  }

  if (auto fecTailMax = captureVdmBerFrameErrorValues(FEC_TAIL_HOST_IN_MAX)) {
    vdmStats.fecTailHostMax() = fecTailMax.value();
  }

  if (auto fecTailCurr = captureVdmBerFrameErrorValues(FEC_TAIL_HOST_IN_CURR)) {
    vdmStats.fecTailHostCurr() = fecTailCurr.value();
  }

  // Fill in VDM Advance group3 performance monitoring info
  if (isVdmSupported(3)) {
    // Lambda to read the VDM PM value for the given VDM Config
    auto getVdmPmLaneValues =
        [&](VdmConfigType vdmConf) -> std::map<int, double> {
      std::map<int, double> pmMap;
      vdmDiagsValLocation = getVdmDiagsValLocation(vdmConf);
      if (vdmDiagsValLocation.vdmConfImplementedByModule) {
        dataAddress = static_cast<int>(vdmDiagsValLocation.vdmValPage);
        offset = vdmDiagsValLocation.vdmValOffset;
        length = vdmDiagsValLocation.vdmValLength;
        data = getQsfpValuePtr(dataAddress, offset, length);
        for (auto lanes = 0; lanes < length / 2; lanes++) {
          double pmVal;
          pmVal = f16ToDouble(data[lanes * 2], data[lanes * 2 + 1]);
          pmMap[lanes] = pmVal;
        }
      }
      return pmMap;
    };

    // PAM4 Level0
    auto sdL0Map = getVdmPmLaneValues(PAM4_LEVEL0_STANDARD_DEVIATION_LINE);
    for (auto [lane, sdL0] : sdL0Map) {
      vdmStats.pam4Level0SDLine()[lane] = sdL0;
    }

    // PAM4 Level1
    auto sdL1Map = getVdmPmLaneValues(PAM4_LEVEL1_STANDARD_DEVIATION_LINE);
    for (auto [lane, sdL1] : sdL1Map) {
      vdmStats.pam4Level1SDLine()[lane] = sdL1;
    }

    // PAM4 Level2
    auto sdL2Map = getVdmPmLaneValues(PAM4_LEVEL2_STANDARD_DEVIATION_LINE);
    for (auto [lane, sdL2] : sdL2Map) {
      vdmStats.pam4Level2SDLine()[lane] = sdL2;
    }

    // PAM4 Level3
    auto sdL3Map = getVdmPmLaneValues(PAM4_LEVEL3_STANDARD_DEVIATION_LINE);
    for (auto [lane, sdL3] : sdL3Map) {
      vdmStats.pam4Level3SDLine()[lane] = sdL3;
    }

    // PAM4 MPI
    auto mpiMap = getVdmPmLaneValues(PAM4_MPI_LINE);
    for (auto [lane, mpi] : mpiMap) {
      vdmStats.pam4MPILine()[lane] = mpi;
    }
  }

  // Fill in channel LTP Media In
  vdmDiagsValLocation = getVdmDiagsValLocation(PAM4_LTP_MEDIA_IN);
  if (vdmDiagsValLocation.vdmConfImplementedByModule) {
    dataAddress = static_cast<int>(vdmDiagsValLocation.vdmValPage);
    offset = vdmDiagsValLocation.vdmValOffset;
    length = vdmDiagsValLocation.vdmValLength;
    data = getQsfpValuePtr(dataAddress, offset, length);
    for (auto lanes = 0; lanes < length / 2; lanes++) {
      double ltp;
      ltp = data[lanes * 2] + (data[lanes * 2 + 1] / kU16TypeLsbDivisor);
      vdmStats.pam4LtpMediaChannel()[lanes] = ltp;
    }
  }

  return vdmStats;
}

/*
 * getVdmPerfMonitorStats
 *
 * This function extracts all VDM info from the VDM specific pages and then
 * returns VDM Performance Monitoring Diags stats.
 */
std::optional<VdmPerfMonitorStats> CmisModule::getVdmPerfMonitorStats() {
  VdmPerfMonitorStats vdmStats;

  if (!isVdmSupported() || !cacheIsValid()) {
    return std::nullopt;
  }

  vdmStats.statsCollectionTme() = WallClockUtil::NowInSecFast();
  vdmStats.intervalStartTime() = vdmIntervalStartTime_;

  if (!fillVdmPerfMonitorSnr(vdmStats)) {
    QSFP_LOG(ERR, this) << "Failed to get VDM Perf Monitor SNR";
  }
  if (!fillVdmPerfMonitorBer(vdmStats)) {
    QSFP_LOG(ERR, this) << "Failed to get VDM Perf Monitor BER";
  }
  if (!fillVdmPerfMonitorFecErr(vdmStats)) {
    QSFP_LOG(ERR, this) << "Failed to get VDM Perf Monitor FEC Error Rate";
  }
  if (!fillVdmPerfMonitorFecTail(vdmStats)) {
    QSFP_LOG(ERR, this) << "Failed to get VDM Perf Monitor FEC Tail";
  }
  if (!fillVdmPerfMonitorLtp(vdmStats)) {
    QSFP_LOG(ERR, this) << "Failed to get VDM Perf Monitor LTP";
  }
  if (!fillVdmPerfMonitorPam4Data(vdmStats)) {
    QSFP_LOG(ERR, this) << "Failed to get VDM Perf Monitor PAM4 data";
  }
  if (!fillVdmPerfMonitorPam4AlarmData(vdmStats)) {
    QSFP_LOG(ERR, this) << "Failed to get VDM Perf Monitor PAM4 alarm data";
  }

  QSFP_LOG(DBG5, this) << "Read VDM Performance Monitoring stats";
  QSFP_LOG(DBG5, this) << "Stats Collection Time: "
                       << vdmStats.statsCollectionTme().value();
  QSFP_LOG(DBG5, this) << "Read " << vdmStats.mediaPortVdmStats()->size()
                       << " ports on media side and "
                       << vdmStats.hostPortVdmStats()->size()
                       << " ports on host side";
  for (auto& [portName, mediaVdmStats] : vdmStats.mediaPortVdmStats().value()) {
    QSFP_LOG(DBG5, this) << "Port: " << portName
                         << " recorded media side stats for "
                         << mediaVdmStats.laneSNR()->size() << " lanes";
  }
  return vdmStats;
}

/*
 * getTunableLaserStatus
 *
 * This function extracts tunable laser status information from the module
 * and returns the laser status and current frequency if available.
 */
std::optional<TunableLaserStatus> CmisModule::getTunableLaserStatus() {
  if (!isTunableOptics()) {
    return std::nullopt;
  }

  TunableLaserStatus tunableLaserStatus;
  // Initialize with default value to avoid bad_optional_field_access
  tunableLaserStatus.tuningStatus() =
      LaserStatusBitMask::LASER_TUNE_NOT_IN_PROGRESS;
  tunableLaserStatus.wavelengthLockingStatus() =
      LaserStatusBitMask::WAVELENGTH_LOCKED;
  tunableLaserStatus.laserFrequencyMhz() = kDefaultFrequencyMhz;
  tunableLaserStatus.laserStatusFlagsByte() = 0;
  // Read laser status from MEDIA_TX_1_LAS_STAT field
  uint8_t laserStatusByte;
  readCmisField(CmisField::MEDIA_TX_1_LAS_STAT, &laserStatusByte);

  // Read laser status flags from MEDIA_TX_1_LAS_STAT_FLAGS field
  uint8_t laserStatusFlagsByte = 0;
  readCmisField(CmisField::MEDIA_TX_1_LAS_STAT_FLAGS, &laserStatusFlagsByte);
  tunableLaserStatus.laserStatusFlagsByte() = laserStatusFlagsByte;

  // Laser status byte bit 7 indicates if the laser is in progress of tuning.
  // Value 0: tuning not in progress, Value 1: tuning in progress
  if (laserStatusByte &
      static_cast<uint8_t>(LaserStatusBitMask::LASER_TUNE_IN_PROGRESS)) {
    tunableLaserStatus.tuningStatus() =
        LaserStatusBitMask::LASER_TUNE_IN_PROGRESS;
  }

  if (laserStatusByte &
      static_cast<uint8_t>(LaserStatusBitMask::WAVELENGTH_UNLOCKED)) {
    tunableLaserStatus.wavelengthLockingStatus() =
        LaserStatusBitMask::WAVELENGTH_UNLOCKED;
  }

  // Read current laser frequency from MEDIA_TX_1_CURR_LAS_FREQ field (4
  // bytes)
  uint8_t frequencyBytes[4] = {0};
  readCmisField(CmisField::MEDIA_TX_1_CURR_LAS_FREQ, frequencyBytes);

  // Convert 4 bytes to frequency in MHz
  // According to CMIS spec, this is stored as a 32-bit unsigned integer in
  // MHz
  uint32_t frequencyMhz = (frequencyBytes[0] << 24) |
      (frequencyBytes[1] << 16) | (frequencyBytes[2] << 8) | frequencyBytes[3];

  tunableLaserStatus.laserFrequencyMhz() = static_cast<int64_t>(frequencyMhz);

  QSFP_LOG(INFO, this) << folly::sformat(
      "Laser status byte: 0x{:X}, laserStatusFlagsByte: 0x{:X}, "
      "frequency_MHz: {}, TuningStatus: {}, WavelengthLockingStatus: {}",
      laserStatusByte,
      laserStatusFlagsByte,
      frequencyMhz,
      apache::thrift::util::enumNameSafe(
          tunableLaserStatus.tuningStatus().value()),
      apache::thrift::util::enumNameSafe(
          tunableLaserStatus.wavelengthLockingStatus().value()));
  return tunableLaserStatus;
}

/*
 * getVdmPerfMonitorStatsForOds
 *
 * Consolidate the VDM stats for publishing to ODS/Fbagent
 * - For Pre FEC BER and Post FEC BER -> Report Max value
 * - For SNR -> Report Min value across all lanes
 * - For PAM4 SD, MPI, LTP -> Report Max value across all lanes
 */
VdmPerfMonitorStatsForOds CmisModule::getVdmPerfMonitorStatsForOds(
    VdmPerfMonitorStats& vdmPerfMonStats) {
  VdmPerfMonitorStatsForOds vdmPerfMonOdsStats;

  vdmPerfMonOdsStats.statsCollectionTme() =
      vdmPerfMonStats.statsCollectionTme().value();

  // Lambda to report Min and Max value from a map of lane id to lane values
  auto findMinMax =
      [](std::map<int32_t, double>& vdmStats) -> std::pair<double, double> {
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
    for (auto& [lane, vdmVal] : vdmStats) {
      if (vdmVal > max) {
        max = vdmVal;
      }
      if (vdmVal < min) {
        min = vdmVal;
      }
    }
    return std::make_pair(min, max);
  };

  // Media side stats consolidation
  for (auto& [portName, portMediaVdmStats] :
       vdmPerfMonStats.mediaPortVdmStats().value()) {
    // Report BER and Post Fec BER, need to report Max only
    vdmPerfMonOdsStats.mediaPortVdmStats()[portName].datapathBERMax() =
        portMediaVdmStats.datapathBER()->max().value();
    vdmPerfMonOdsStats.mediaPortVdmStats()[portName]
        .datapathErroredFramesMax() =
        portMediaVdmStats.datapathErroredFrames()->max().value();

    if (auto fecTailMax = portMediaVdmStats.fecTailMax()) {
      vdmPerfMonOdsStats.mediaPortVdmStats()[portName].fecTailMax() =
          fecTailMax.value();
    }

    // For SNR, report Min value among all lanes
    vdmPerfMonOdsStats.mediaPortVdmStats()[portName].laneSNRMin() =
        findMinMax(portMediaVdmStats.laneSNR().value()).first;

    // For PAM4 SD, MPI, LTP, report Max value among all lanes
    vdmPerfMonOdsStats.mediaPortVdmStats()[portName].lanePam4Level0SDMax() =
        findMinMax(portMediaVdmStats.lanePam4Level0SD().value()).second;
    vdmPerfMonOdsStats.mediaPortVdmStats()[portName].lanePam4Level1SDMax() =
        findMinMax(portMediaVdmStats.lanePam4Level1SD().value()).second;
    vdmPerfMonOdsStats.mediaPortVdmStats()[portName].lanePam4Level2SDMax() =
        findMinMax(portMediaVdmStats.lanePam4Level2SD().value()).second;
    vdmPerfMonOdsStats.mediaPortVdmStats()[portName].lanePam4Level3SDMax() =
        findMinMax(portMediaVdmStats.lanePam4Level3SD().value()).second;
    vdmPerfMonOdsStats.mediaPortVdmStats()[portName].lanePam4MPIMax() =
        findMinMax(portMediaVdmStats.lanePam4MPI().value()).second;
    vdmPerfMonOdsStats.mediaPortVdmStats()[portName].lanePam4LTPMax() =
        findMinMax(portMediaVdmStats.lanePam4LTP().value()).second;
  }

  // Host side stats consolidation
  for (auto& [portName, portHostVdmStats] :
       vdmPerfMonStats.hostPortVdmStats().value()) {
    // Report BER and Post Fec BER, need to report Max only
    vdmPerfMonOdsStats.hostPortVdmStats()[portName].datapathBERMax() =
        portHostVdmStats.datapathBER()->max().value();
    vdmPerfMonOdsStats.hostPortVdmStats()[portName].datapathErroredFramesMax() =
        portHostVdmStats.datapathErroredFrames()->max().value();

    if (auto fecTailMax = portHostVdmStats.fecTailMax()) {
      vdmPerfMonOdsStats.hostPortVdmStats()[portName].fecTailMax() =
          *fecTailMax;
    }
  }

  return vdmPerfMonOdsStats;
}

TransceiverModuleIdentifier CmisModule::getIdentifier() {
  return (TransceiverModuleIdentifier)getSettingsValue(CmisField::IDENTIFIER);
}

void CmisModule::setQsfpFlatMem() {
  uint8_t flatMem;
  int offset;
  int length;
  int dataAddress;

  if (!present_) {
    throw FbossError("Failed setting QSFP flatMem: QSFP is not present");
  }

  getQsfpFieldAddress(CmisField::FLAT_MEM, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, &flatMem);
  flatMem_ = flatMem & (1 << 7);
  QSFP_LOG(DBG3, this) << "Detected QSFP, flatMem=" << flatMem_;
}

const uint8_t*
CmisModule::getQsfpValuePtr(int dataAddress, int offset, int length) const {
  /* if the cached values are not correct */
  if (!cacheIsValid()) {
    throw FbossError("Qsfp is either not present or the data is not read");
  }
  if (dataAddress == static_cast<int>(CmisPages::LOWER)) {
    CHECK_LE(offset + length, sizeof(lowerPage_));
    /* Copy data from the cache */
    return (lowerPage_ + offset);
  } else {
    offset -= MAX_QSFP_PAGE_SIZE;
    CHECK_GE(offset, 0);
    CHECK_LE(offset, MAX_QSFP_PAGE_SIZE);

    // If this is a flatMem module, we will only have PAGE00 here.
    // Only when flatMem is false will we have data for other pages.

    if (dataAddress == static_cast<int>(CmisPages::PAGE00)) {
      CHECK_LE(offset + length, sizeof(page0_));
      return (page0_ + offset);
    }

    if (flatMem_) {
      throw FbossError(
          "Accessing upper page ", dataAddress, " on flatMem module.");
    }

    switch (static_cast<CmisPages>(dataAddress)) {
      case CmisPages::PAGE01:
        CHECK_LE(offset + length, sizeof(page01_));
        return (page01_ + offset);
      case CmisPages::PAGE02:
        CHECK_LE(offset + length, sizeof(page02_));
        return (page02_ + offset);
      case CmisPages::PAGE04:
        CHECK_LE(offset + length, sizeof(page04_));
        return (page04_ + offset);
      case CmisPages::PAGE10:
        CHECK_LE(offset + length, sizeof(page10_));
        return (page10_ + offset);
      case CmisPages::PAGE11:
        CHECK_LE(offset + length, sizeof(page11_));
        return (page11_ + offset);
      case CmisPages::PAGE12:
        CHECK_LE(offset + length, sizeof(page12_));
        return (page12_ + offset);
      case CmisPages::PAGE13:
        CHECK_LE(offset + length, sizeof(page13_));
        return (page13_ + offset);
      case CmisPages::PAGE14:
        CHECK_LE(offset + length, sizeof(page14_));
        return (page14_ + offset);
      case CmisPages::PAGE20:
        CHECK_LE(offset + length, sizeof(page20_));
        return (page20_ + offset);
      case CmisPages::PAGE21:
        CHECK_LE(offset + length, sizeof(page21_));
        return (page21_ + offset);
      case CmisPages::PAGE22:
        CHECK_LE(offset + length, sizeof(page22_));
        return (page22_ + offset);
      case CmisPages::PAGE24:
        CHECK_LE(offset + length, sizeof(page24_));
        return (page24_ + offset);
      case CmisPages::PAGE25:
        CHECK_LE(offset + length, sizeof(page25_));
        return (page25_ + offset);
      case CmisPages::PAGE26:
        CHECK_LE(offset + length, sizeof(page26_));
        return (page26_ + offset);
      default:
        throw FbossError("Invalid Data Address 0x%d", dataAddress);
    }
  }
}

/*
 * readFromCacheOrHw
 *
 * This function reads the register field from either register cache or from
 * hardware (if the cache is not available). This function assumes the input
 * data pointer has the space allocated for the entire given CMIS register space
 */
void CmisModule::readFromCacheOrHw(
    CmisField field,
    uint8_t* data,
    bool forcedReadFromHw) {
  int offset;
  int length;
  int dataAddress;
  getQsfpFieldAddress(field, dataAddress, offset, length);
  if (cacheIsValid() && !forcedReadFromHw) {
    getQsfpValue(dataAddress, offset, length, data);
  } else {
    readCmisField(field, data);
  }
}

RawDOMData CmisModule::getRawDOMData() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  RawDOMData data;
  if (present_) {
    *data.lower() = IOBuf::wrapBufferAsValue(lowerPage_, MAX_QSFP_PAGE_SIZE);
    *data.page0() = IOBuf::wrapBufferAsValue(page0_, MAX_QSFP_PAGE_SIZE);
    data.page10() = IOBuf::wrapBufferAsValue(page10_, MAX_QSFP_PAGE_SIZE);
    data.page11() = IOBuf::wrapBufferAsValue(page11_, MAX_QSFP_PAGE_SIZE);
  }
  return data;
}

DOMDataUnion CmisModule::getDOMDataUnion() {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  CmisData cmisData;
  if (present_) {
    *cmisData.lower() =
        IOBuf::wrapBufferAsValue(lowerPage_, MAX_QSFP_PAGE_SIZE);
    *cmisData.page0() = IOBuf::wrapBufferAsValue(page0_, MAX_QSFP_PAGE_SIZE);
    if (!flatMem_) {
      cmisData.page01() = IOBuf::wrapBufferAsValue(page01_, MAX_QSFP_PAGE_SIZE);
      cmisData.page02() = IOBuf::wrapBufferAsValue(page02_, MAX_QSFP_PAGE_SIZE);
      cmisData.page10() = IOBuf::wrapBufferAsValue(page10_, MAX_QSFP_PAGE_SIZE);
      cmisData.page11() = IOBuf::wrapBufferAsValue(page11_, MAX_QSFP_PAGE_SIZE);
      if (isTunableOptics()) {
        cmisData.page12() =
            IOBuf::wrapBufferAsValue(page12_, MAX_QSFP_PAGE_SIZE);
      }
      cmisData.page13() = IOBuf::wrapBufferAsValue(page13_, MAX_QSFP_PAGE_SIZE);
      cmisData.page14() = IOBuf::wrapBufferAsValue(page14_, MAX_QSFP_PAGE_SIZE);
      cmisData.page20() = IOBuf::wrapBufferAsValue(page20_, MAX_QSFP_PAGE_SIZE);
      cmisData.page21() = IOBuf::wrapBufferAsValue(page21_, MAX_QSFP_PAGE_SIZE);
      cmisData.page22() = IOBuf::wrapBufferAsValue(page22_, MAX_QSFP_PAGE_SIZE);
      cmisData.page24() = IOBuf::wrapBufferAsValue(page24_, MAX_QSFP_PAGE_SIZE);
      cmisData.page25() = IOBuf::wrapBufferAsValue(page25_, MAX_QSFP_PAGE_SIZE);
      cmisData.page26() = IOBuf::wrapBufferAsValue(page26_, MAX_QSFP_PAGE_SIZE);
    }
  }
  cmisData.timeCollected() = lastRefreshTime_;
  DOMDataUnion data;
  data.cmis() = cmisData;
  return data;
}

void CmisModule::getFieldValue(CmisField fieldName, uint8_t* fieldValue) const {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  int offset;
  int length;
  int dataAddress;
  getQsfpFieldAddress(fieldName, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, fieldValue);
}

void CmisModule::getFieldValueLocked(CmisField fieldName, uint8_t* fieldValue)
    const {
  // Expect lock being held here.
  int offset;
  int length;
  int dataAddress;
  getQsfpFieldAddress(fieldName, dataAddress, offset, length);
  getQsfpValue(dataAddress, offset, length, fieldValue);
}

void CmisModule::updateQsfpData(bool allPages) {
  // expects the lock to be held
  if (!present_) {
    return;
  }
  try {
    QSFP_LOG(DBG2, this) << "Performing " << ((allPages) ? "full" : "partial")
                         << " qsfp data cache refresh";
    readCmisField(CmisField::PAGE_LOWER, lowerPage_);
    lastRefreshTime_ = std::time(nullptr);
    dirty_ = false;
    setQsfpFlatMem();

    readCmisField(CmisField::PAGE_UPPER00H, page0_);
    if (!flatMem_) {
      readCmisField(CmisField::PAGE_UPPER10H, page10_);
      readCmisField(CmisField::PAGE_UPPER11H, page11_);
      if (isTunableOptics()) {
        readCmisField(CmisField::PAGE_UPPER12H, page12_);
      }

      bool isReady =
          ((CmisModuleState)(getSettingsValue(CmisField::MODULE_STATE) >> 1) ==
           CmisModuleState::READY);
      if (isReady) {
        auto diagFeature = (uint8_t)DiagnosticFeatureEncoding::SNR;
        writeCmisField(CmisField::DIAG_SEL, &diagFeature);
        readCmisField(CmisField::PAGE_UPPER14H, page14_);
        updateVdmCacheLocked();
      }
    }

    if (!allPages) {
      // Update the application capabilities once we have read from eeprom.
      // Note that this function may also need to read information from page01
      // which is only read when allPages_ is true. However, we always read
      // allPages the first time so we'll have cached information of page01
      // (when applicable) by now. That information is also static so it's okay
      // that we are not reading it again.
      getApplicationCapabilities();
      // The information on the following pages are static. Thus no need to
      // fetch them every time. We just need to do it when we first retriving
      // the data from this module.
      return;
    }

    if (!flatMem_) {
      readCmisField(CmisField::PAGE_UPPER01H, page01_);
      readCmisField(CmisField::PAGE_UPPER02H, page02_);
      readCmisField(CmisField::PAGE_UPPER13H, page13_);
    }

    // Update the application capabilities once we have read from eeprom
    getApplicationCapabilities();
  } catch (const std::exception& ex) {
    // No matter what kind of exception throws, we need to set the dirty_ flag
    // to true.
    dirty_ = true;
    QSFP_LOG(ERR, this) << "Error update data: " << ex.what();
    throw;
  }
}

/*
 * setApplicationSelectCode
 *
 * Set the Application code to the optics for just one software port. If it
 * needs cleanup of existing config first then the lanes are released first
 * before programming new application select code
 */
void CmisModule::setApplicationSelectCode(
    uint8_t apSelCode,
    uint8_t mediaInterfaceCode,
    uint8_t startHostLane,
    uint8_t numHostLanes,
    uint8_t hostLaneMask) {
  uint8_t dataPathId = startHostLane;
  uint8_t explicitControl = 0; // Use application dependent settings
  uint8_t newApSelCode = (apSelCode << 4) | (dataPathId << 1) | explicitControl;
  QSFP_LOG(INFO, this) << folly::sformat("newApSelCode: {:#x}", newApSelCode);

  // We can't use numHostLanes() to get the hostLaneCount here since
  // that function relies on the configured application select but at
  // this point appSel hasn't been updated.
  uint8_t applySetForConfigureLanes = hostLaneMask;
  uint8_t applySetForReleaseLanes = 0;

  std::set<uint8_t> lanesToRelease;
  // Read and cache all laneToActiveCtrlField. We can't rely on existing
  // cache because we may not have got a chance to update in between
  // programming different ports in a sequence
  std::array<uint8_t, 8> laneToActiveCtrlFieldVals{};
  readCmisField(
      CmisField::ACTIVE_CTRL_ALL_LANES, laneToActiveCtrlFieldVals.data());

  for (uint8_t lane = startHostLane; lane < startHostLane + numHostLanes;
       lane++) {
    lanesToRelease.insert(lane);
    applySetForReleaseLanes |= (1 << lane);
    // Get all lanes with the same data path ID as this lane
    uint8_t currDataPathId =
        (laneToActiveCtrlFieldVals[lane] & DATA_PATH_ID_MASK) >>
        DATA_PATH_ID_BITSHIFT;
    uint8_t currAppSel =
        (laneToActiveCtrlFieldVals[lane] & APP_SEL_MASK) >> APP_SEL_BITSHIFT;
    // If currently App Sel is 0, it means this lane is not part of any
    // active data path yet. No need to find other lanes to release
    if (currAppSel == 0) {
      continue;
    }
    // If we are here, it means that this lane is part of an active data
    // path. Find out which other lanes are active with the same data path
    // id and then release them
    for (auto it = laneToActiveCtrlField.begin();
         it != laneToActiveCtrlField.end();
         it++) {
      auto otherLane = it->first;
      uint8_t otherAppSel =
          (laneToActiveCtrlFieldVals[otherLane] & APP_SEL_MASK) >>
          APP_SEL_BITSHIFT;
      // Ignore lanes with app sel 0 as that means that the lane is not part
      // of any data path
      if (otherAppSel == 0) {
        continue;
      }
      uint8_t otherDataPathId =
          (laneToActiveCtrlFieldVals[otherLane] & DATA_PATH_ID_MASK) >>
          DATA_PATH_ID_BITSHIFT;
      if (currDataPathId == otherDataPathId) {
        lanesToRelease.insert(otherLane);
        applySetForReleaseLanes |= (1 << otherLane);
      }
    }
  }

  // First release the lanes if they are already part of any datapath
  std::vector<uint8_t> zeroApSelCode(lanesToRelease.size(), 0);
  for (auto it = lanesToRelease.begin(); it != lanesToRelease.end(); it++) {
    QSFP_LOG(INFO, this) << folly::sformat("Releasing lane {:#x}", *it);
  }
  writeCmisField(laneToAppSelField(lanesToRelease), zeroApSelCode.data());
  // We don't need to check if lanesToRelease is empty or not before setting
  // stage_ctrl_set_0 because there will always be lanes to release. At the
  // minimum, we'll try to release the same lane we are trying to configure
  writeCmisField(CmisField::STAGE_CTRL_SET_0, &applySetForReleaseLanes);

  std::set<uint8_t> lanesToProgramAppSel;
  std::vector<uint8_t> appSelCode;
  for (uint8_t lane = startHostLane; lane < startHostLane + numHostLanes;
       lane++) {
    // Assign ApSel code to each lane
    QSFP_LOG(INFO, this) << folly::sformat(
        "Configuring lane {:#x} with apsel code {:#x}", lane, newApSelCode);
    lanesToProgramAppSel.insert(lane);
    appSelCode.push_back(newApSelCode);
  }
  writeCmisField(laneToAppSelField(lanesToProgramAppSel), appSelCode.data());

  writeCmisField(CmisField::STAGE_CTRL_SET_0, &applySetForConfigureLanes);

  datapathResetPendingMask_ = applySetForConfigureLanes;

  QSFP_LOG(INFO, this) << folly::sformat(
      "set application to {:#x}", mediaInterfaceCode);
}

/*
 * setApplicationSelectCodeAllPorts
 *
 * This function programs the application select code on all the software port
 * for a given optics. This is required when the optics has to transition to a
 * valid configuration for all the lanes
 */
void CmisModule::setApplicationSelectCodeAllPorts(
    cfg::PortSpeed speed,
    uint8_t startHostLane,
    uint8_t numHostLanes,
    uint8_t hostLaneMask) {
  std::vector<uint8_t> laneProgramValues;
  if (isAecModule()) {
    laneProgramValues =
        CmisHelper::getValidMultiportSpeedConfig<ActiveCuHostInterfaceCode>(
            speed,
            startHostLane,
            numHostLanes,
            laneMask(startHostLane, numHostLanes),
            getNameString(),
            moduleCapabilities_,
            CmisHelper::getActiveValidSpeedCombinations(),
            CmisHelper::getActiveSpeedApplication());
  } else {
    laneProgramValues =
        CmisHelper::getValidMultiportSpeedConfig<SMFMediaInterfaceCode>(
            speed,
            startHostLane,
            numHostLanes,
            laneMask(startHostLane, numHostLanes),
            getNameString(),
            moduleCapabilities_,
            CmisHelper::getSmfValidSpeedCombinations(),
            CmisHelper::getSmfSpeedApplicationMapping());
  }
  if (laneProgramValues.size() == kMaxOsfpNumLanes) {
    AllLaneConfig stageSet0Config;
    for (auto lane = 0; lane < kMaxOsfpNumLanes;) {
      if (auto laneCapability =
              getApplicationField(laneProgramValues[lane], lane)) {
        uint8_t currApSelCode = laneCapability.value().ApSelCode;
        for (auto currApLane = lane;
             currApLane < lane + laneCapability.value().hostLaneCount;
             currApLane++) {
          stageSet0Config[currApLane] = currApSelCode << APP_SEL_BITSHIFT |
              (lane << DATA_PATH_ID_BITSHIFT);
        }
        lane += laneCapability.value().hostLaneCount;
      } else {
        stageSet0Config[lane++] = 0;
      }
    }
    writeCmisField(CmisField::APP_SEL_LANE_1_8, stageSet0Config.data());

    // Trigger the Set 0 application code setting to be applied on data
    // path init for all the lanes. The actual data-path init will be
    // triggered from the caller function
    uint8_t applySetForSpecificLanes = laneMask(0, kMaxOsfpNumLanes);
    writeCmisField(CmisField::STAGE_CTRL_SET_0, &applySetForSpecificLanes);

    datapathResetPendingMask_ = applySetForSpecificLanes;
  }
}

/*
 * setMaxFecSamplingLocked
 *
 * Sets the FEC monitor sampling ratio to maximum.
 * Datapath state or module operation would not be interrupted during this
 * configuration
 */
void CmisModule::setMaxFecSamplingLocked() {
  // FLAGS_set_max_fec_sampling is used to roll out the feature
  if (FLAGS_set_max_fec_sampling) {
    auto mediaInterface = getModuleMediaInterface();
    auto itr = kMaxProgFecSamplingSupportedMap_.find(mediaInterface);
    if (itr != kMaxProgFecSamplingSupportedMap_.end()) {
      uint8_t max = itr->second;
      writeCmisField(CmisField::FEC_SAMPLING_PCT, &max);
      QSFP_LOG(INFO, this) << folly::sformat(
          "set sampling rate to max: {} for module media interface {}",
          max,
          apache::thrift::util::enumNameSafe(mediaInterface));
    }
  }
}

/*
 * programApplicationSelectCode
 *
 * Helper function to program a given AppSel code to the module. This contains
 * the common logic for resetting datapath, programming the AppSel code,
 * waiting for the module to process, and verifying the configuration.
 *
 * If appSelectFunc is provided, it will be used. Otherwise, the default
 * setApplicationSelectCode will be used.
 */
void CmisModule::programApplicationSelectCode(
    uint8_t appSelCode,
    uint8_t moduleMediaInterfaceCode,
    uint8_t startHostLane,
    uint8_t numHostLanes,
    std::optional<std::function<void()>> appSelectFunc) {
  uint8_t hostLaneMask = laneMask(startHostLane, numHostLanes);

  // Use provided function or create default one
  if (!appSelectFunc) {
    QSFP_LOG(INFO, this) << folly::sformat(
        "Programming App sel on lanes {:#x}", hostLaneMask);

    appSelectFunc = std::bind(
        &CmisModule::setApplicationSelectCode,
        this,
        appSelCode,
        moduleMediaInterfaceCode,
        startHostLane,
        numHostLanes,
        hostLaneMask);
  }

  resetDataPathWithFunc(appSelectFunc, hostLaneMask);

  datapathResetPendingMask_ &= ~hostLaneMask;

  // Certain OSFP Modules require a long time to finish application
  // programming. The modules say config is accepted and applied, but
  // internally the module will still be processing the config. If we don't
  // have a delay here, the next application programming on a different lane
  // gets rejected.
  /* sleep override */
  usleep(kUsecAfterAppProgramming);

  // Check if the config has been applied correctly or not
  // TODO: This is a failure scenario. We should Fail somehow !
  if (!checkLaneConfigError(startHostLane, numHostLanes)) {
    QSFP_LOG(ERR, this) << folly::sformat(
        "application {:#x} could not be set", moduleMediaInterfaceCode);
  }
}

/*
 * getInterfaceCodeForAppSel
 *
 * Helper function to read an interface code (host or media) for a given AppSel
 * code from the EEPROM based on the byte offset.
 * - For media interface code, use byteOffset = kMediaInterfaceCodeOffset (1)
 * - For host interface code, use byteOffset = kHostInterfaceCodeOffset (0)
 */
uint8_t CmisModule::getInterfaceCodeForAppSel(
    uint8_t appSelCode,
    int byteOffset) {
  if (appSelCode == 0 || appSelCode > 15) {
    QSFP_LOG(INFO, this) << folly::sformat(
        "Invalid appSelCode {:#x}, returning 0", appSelCode);
    return 0;
  }

  uint8_t interfaceCode = 0;
  int offset;
  int length;
  int dataAddress;

  if (appSelCode >= 1 && appSelCode <= 8) {
    getQsfpFieldAddress(
        CmisField::APPLICATION_ADVERTISING1, dataAddress, offset, length);
    offset += (appSelCode - 1) * length + byteOffset;
    getQsfpValue(dataAddress, offset, 1, &interfaceCode);
  } else if (appSelCode >= 9 && appSelCode <= 15) {
    getQsfpFieldAddress(
        CmisField::APPLICATION_ADVERTISING2, dataAddress, offset, length);
    offset += (appSelCode - 9) * length + byteOffset;
    getQsfpValue(dataAddress, offset, 1, &interfaceCode);
  }

  QSFP_LOG(INFO, this) << folly::sformat(
      "interfaceCode: {:#x} for appSelCode {:#x} byteOffset {}",
      interfaceCode,
      appSelCode,
      byteOffset);

  return interfaceCode;
}

/*
 * getCurrentAppSelCode
 *
 * Helper function to read the current application select code for a given lane.
 */
uint8_t CmisModule::getCurrentAppSelCode(uint8_t startHostLane) {
  uint8_t currentApplicationSel =
      getSettingsValue(laneToActiveCtrlField[startHostLane], APP_SEL_MASK);
  return currentApplicationSel >> APP_SEL_BITSHIFT;
}

/*
 * getAppSelCodeForSpeed
 *
 * Helper function that discovers and returns the appropriate application
 * capability based on module capabilities and speed requirements. Returns
 * the full ApplicationAdvertisingField if a suitable application is found,
 * or std::nullopt if no matching application is available or if the current
 * config already matches.
 */
std::optional<CmisModule::ApplicationAdvertisingField>
CmisModule::getAppSelCodeForSpeed(
    cfg::PortSpeed speed,
    uint8_t startHostLane,
    uint8_t numHostLanesForPort) {
  std::vector<uint8_t> appCodes;

  if (isAecModule()) {
    appCodes = CmisHelper::getInterfaceCode<ActiveCuHostInterfaceCode>(
        speed, CmisHelper::getActiveSpeedApplication());
  } else {
    appCodes = CmisHelper::getInterfaceCode<SMFMediaInterfaceCode>(
        speed, CmisHelper::getSmfSpeedApplicationMapping());
  }

  if (appCodes.empty()) {
    QSFP_LOG(INFO, this) << "Unsupported Speed.";
    throw FbossError(
        folly::to<std::string>(
            "Transceiver: ",
            qsfpImpl_->getName(),
            " Unsupported speed: ",
            apache::thrift::util::enumNameSafe(speed)));
  }

  QSFP_LOG(INFO, this) << "Application codes supporting current speed: "
                       << folly::join(",", appCodes);

  // Currently we will have the same application across all the lanes. So here
  // we only take one of them to look at.
  uint8_t currentApplicationSel = getCurrentAppSelCode(startHostLane);

  QSFP_LOG(INFO, this) << folly::sformat(
      "currentApplicationSel: {:#x} speed {:s} startHostLane {:d} numHostLanesForPort {:d}",
      currentApplicationSel,
      apache::thrift::util::enumNameSafe(speed),
      startHostLane,
      numHostLanesForPort);

  // We use the module Media Interface ID for Optical modules, which is located
  // at the second byte of the field (byteOffset = 1), as Application ID here.
  // If we have an AEC cable, we use the module host interface ID which has a
  // byteOffset of 0. This is in page 00h app sel advertising (starting at
  // offset 86 for page)
  int byteOffset =
      isAecModule() ? kHostInterfaceCodeOffset : kMediaInterfaceCodeOffset;

  // Get the current application interface code based on module type
  uint8_t currentApplication = 0;
  if (currentApplicationSel >= 1 && currentApplicationSel <= 15) {
    currentApplication =
        getInterfaceCodeForAppSel(currentApplicationSel, byteOffset);
  } else {
    QSFP_LOG(INFO, this) << "currentApplication: not selected yet";
  }

  // Loop through all the applications that we support for the given speed and
  // check if any of those are present in the moduleCapabilities. We configure
  // the first application that both we support and the module supports
  for (auto application : appCodes) {
    auto capability = getApplicationField(application, startHostLane);

    // Check if the module supports the application
    if (!capability) {
      continue;
    }

    auto numHostLanes = capability->hostLaneCount;
    // We could support 100G-1 or 100G-4, 200G-1, 200G-2 or 200G-4, 400G-2 or
    // 400G-4. Thus compare both speed and number of host lanes
    if ((speed == cfg::PortSpeed::HUNDREDG ||
         speed == cfg::PortSpeed::TWOHUNDREDG ||
         speed == cfg::PortSpeed::FOURHUNDREDG) &&
        numHostLanesForPort != numHostLanes) {
      continue;
    }

    // If the currently configured application is the same as what we are trying
    // to configure, then skip the configuration
    if (application == currentApplication) {
      QSFP_LOG(INFO, this) << folly::sformat(
          "Speed matches: currentApplication {:#x}. Doing nothing",
          currentApplication);
      // Make sure the datapath is initialized, otherwise initialize it before
      // returning
      uint8_t hostLaneMask = laneMask(startHostLane, numHostLanes);
      if (datapathResetPendingMask_ & hostLaneMask) {
        resetDataPathWithFunc(std::nullopt, hostLaneMask);
        datapathResetPendingMask_ &= ~hostLaneMask;
        QSFP_LOG(INFO, this) << folly::sformat(
            "Reset datapath for lane mask {:#x} before returning",
            hostLaneMask);
      }
      return std::nullopt;
    }

    // Found a matching application - return the full capability
    return *capability;
  }

  // We didn't find an application that both we support and the module supports
  QSFP_LOG(INFO, this) << "Unsupported Application";
  throw FbossError(
      folly::to<std::string>(
          "Port: ",
          qsfpImpl_->getName(),
          " Unsupported Application by the module: "));
}

/*
 * setApplicationCodeLocked
 *
 * This function programs the application select code for a port using the speed
 * value, start lane number and number of lanes. It goes through module's
 * advertised media interface support capabilities to find appropriate
 * application code tp program. If required, it programs valid configuration on
 * other lanes of the module also.
 */
void CmisModule::setApplicationCodeLocked(
    cfg::PortSpeed speed,
    uint8_t startHostLane,
    uint8_t numHostLanesForPort,
    uint8_t newAppSelCode) {
  QSFP_LOG(INFO, this) << folly::sformat(
      "Trying to set application code for speed {} on startHostLane {}",
      apache::thrift::util::enumNameSafe(speed),
      startHostLane);

  // For tunable optics, directly program the AppSel code from config
  if (isTunableOptics()) {
    if (newAppSelCode == kInvalidApplication) {
      throw FbossError("newAppSelCode is invalid for tunable optics");
    }

    QSFP_LOG(INFO, this) << folly::sformat(
        "Direct AppSelCode programming for speed {} on startHostLane {} newAppSelCode {}",
        apache::thrift::util::enumNameSafe(speed),
        startHostLane,
        newAppSelCode);

    // Check if current AppSel matches the desired one
    uint8_t currentAppSelCode = getCurrentAppSelCode(startHostLane);
    if (currentAppSelCode == newAppSelCode) {
      QSFP_LOG(INFO, this) << folly::sformat(
          "AppSel code matches: current {:#x} new {:#x}, skipping programming",
          currentAppSelCode,
          newAppSelCode);
      return;
    }

    // Get the media interface code and program the AppSel
    uint8_t moduleMediaInterfaceCode =
        getInterfaceCodeForAppSel(newAppSelCode, kMediaInterfaceCodeOffset);

    programApplicationSelectCode(
        newAppSelCode,
        moduleMediaInterfaceCode,
        startHostLane,
        numHostLanesForPort);
    return;
  }

  // For non-tunable optics, discover the AppSel code based on capabilities
  auto capability =
      getAppSelCodeForSpeed(speed, startHostLane, numHostLanesForPort);

  // If nullopt, means current config already matches, nothing to do
  if (!capability) {
    return;
  }

  uint8_t appSelCode = capability->ApSelCode;
  uint8_t moduleMediaInterfaceCode = capability->moduleMediaInterface;
  uint8_t numHostLanes = capability->hostLaneCount;

  // Handle special OSFP multiport case
  // In 400G-FR4 case we will have 8 host lanes instead of 4. Further more,
  // we need to deactivate all the lanes when we switch to an application with
  // a different lane count. CMIS4.0-8.8.4
  std::optional<std::function<void()>> appSelectFunc = std::nullopt;

  if (getIdentifier() == TransceiverModuleIdentifier::OSFP &&
      !isRequestValidMultiportSpeedConfig(speed, startHostLane, numHostLanes)) {
    QSFP_LOG(INFO, this) << "Programming App sel on ALL lanes";
    uint8_t hostLaneMask = laneMask(startHostLane, numHostLanes);
    appSelectFunc = std::bind(
        &CmisModule::setApplicationSelectCodeAllPorts,
        this,
        speed,
        startHostLane,
        numHostLanes,
        hostLaneMask);
  }

  // Use programApplicationSelectCode for both cases
  programApplicationSelectCode(
      appSelCode,
      moduleMediaInterfaceCode,
      startHostLane,
      numHostLanes,
      appSelectFunc);
}

/*
 * isRequestValidMultiportSpeedConfig
 *
 * This function returns if the requested speed on given number of lanes will
 * result in valid config on the overall optics. If the requested config on
 * given lanes will result in non-supported speed config (as described in static
 * list getSmfValidSpeedCombinations) then this function returns false otherwise
 * returns true. This function does not rely on cache and does the directHW read
 * to know the current speed config on the lanes.
 */
bool CmisModule::isRequestValidMultiportSpeedConfig(
    cfg::PortSpeed speed,
    uint8_t startHostLane,
    uint8_t numLanes) {
  if (!isMultiPortOptics()) {
    // For non-multiport supporting optics, return true rightaway
    return true;
  }

  // Get the current speed config on the Multiport optics lanes. Avoid cache
  // and read from HW directly
  AllLaneConfig currHwSpeedConfig;
  readCmisField(CmisField::ACTIVE_CTRL_ALL_LANES, currHwSpeedConfig.data());
  for (int laneId = 0; laneId < kMaxOsfpNumLanes; laneId++) {
    currHwSpeedConfig[laneId] =
        (currHwSpeedConfig[laneId] & APP_SEL_MASK) >> APP_SEL_BITSHIFT;
  }

  uint8_t mask = laneMask(startHostLane, numLanes);
  auto tcvrName = getNameString();

  if (isAecModule()) {
    return CmisHelper::checkSpeedCombo<ActiveCuHostInterfaceCode>(
        speed,
        startHostLane,
        numLanes,
        mask,
        tcvrName,
        moduleCapabilities_,
        currHwSpeedConfig,
        CmisHelper::getActiveValidSpeedCombinations(),
        CmisHelper::getActiveSpeedApplication());
  } else {
    return CmisHelper::checkSpeedCombo<SMFMediaInterfaceCode>(
        speed,
        startHostLane,
        numLanes,
        mask,
        tcvrName,
        moduleCapabilities_,
        currHwSpeedConfig,
        CmisHelper::getSmfValidSpeedCombinations(),
        CmisHelper::getSmfSpeedApplicationMapping());
  }
}

/*
 * This function checks if the previous lane configuration has been successul
 * or rejected. It will log error and return false if config on a lane is
 * rejected. This function should be run after ApSel setting or any other
 * lane configuration like Rx Equalizer setting etc
 */
bool CmisModule::checkLaneConfigError(
    uint8_t startHostLane,
    uint8_t hostLaneCount) {
  bool success;

  uint8_t configErrors[4];

  // In case some channel information is not available then we can retry after
  // lane init time
  int retryCount = 2;
  while (retryCount) {
    retryCount--;
    readCmisField(CmisField::CONFIG_ERROR_LANES, configErrors);

    bool allStatusAvailable = true;
    success = true;

    for (int channel = startHostLane; channel < startHostLane + hostLaneCount;
         channel++) {
      uint8_t byte = channel / 2;
      uint8_t cfgErr = configErrors[byte] >> ((channel % 2) * 4);
      cfgErr &= 0x0f;
      if (cfgErr >= 8) {
        cfgErr = 8;
      }
      // If some lane info is not available then we need to try again
      if (cfgErr == 0) {
        allStatusAvailable = false;
      }
      // Status other than 1 is considered failed
      if (cfgErr != 1) {
        success = false;
      }
      QSFP_LOG(INFO, this) << folly::sformat(
          "Lane {:d} config stats: {:s}",
          channel,
          channelConfigErrorMsg[cfgErr]);
    }

    // If all channel information is available then no need to retry and break
    // from this loop, otherwise retry one more time after a wait period
    if (allStatusAvailable) {
      break;
    } else if (retryCount) {
      QSFP_LOG(INFO, this) << "Some lane status not available so trying again";
      /* sleep override */
      usleep(kUsecBetweenLaneInit);
    } else {
      QSFP_LOG(ERR, this) << "Some lane status not available even after retry";
    }
  };

  return success;
}

/*
 * Put logic here that should only be run on ports that have been
 * down for a long time. These are actions that are potentially more
 * disruptive, but have worked in the past to recover a transceiver.
 */
void CmisModule::remediateFlakyTransceiver(
    bool allPortsDown,
    const std::vector<std::string>& ports) {
  QSFP_LOG(INFO, this) << "allPortsDown = " << allPortsDown
                       << ". Performing potentially disruptive remediations on "
                       << folly::join(",", ports);

  if (allPortsDown) {
    // This api accept 1 based module id however the module id in WedgeManager
    // is 0 based.
    triggerModuleReset();
  } else {
    auto portNameToHostLanesMap = getPortNameToHostLanes();
    for (const auto& port : ports) {
      if (portNameToHostLanesMap.find(port) != portNameToHostLanesMap.end()) {
        auto& lanes = portNameToHostLanesMap[port];
        if (!lanes.empty()) {
          auto portLaneMask = laneMask(*lanes.begin(), lanes.size());
          QSFP_LOG(INFO, this)
              << "Doing datapath reinit for " << port << " with lane mask "
              << static_cast<int>(portLaneMask);
          resetDataPathWithFunc(std::nullopt, portLaneMask);
        } else {
          QSFP_LOG(ERR, this) << "Host lanes empty for " << port
                              << ". Skipping individual datapath remediation.";
        }
      } else {
        QSFP_LOG(ERR, this) << "Host lanes unavailable for " << port
                            << ". Skipping individual datapath remediation.";
      }
    }
  }

  // Reset lastRemediateTime_ so we can use cool down before next remediation
  lastRemediateTime_ = std::time(nullptr);
}

void CmisModule::setPowerOverrideIfSupportedLocked(
    PowerControlState currentState) {
  /* Wedge forces Low Power mode via a pin;  we have to reset this
   * to force High Power mode on all transceivers except SR4-40G.
   *
   * Note that this function expects to be called with qsfpModuleMutex_
   * held.
   */

  int offset;
  int length;
  int dataAddress;

  if (currentState == PowerControlState::HIGH_POWER_OVERRIDE) {
    QSFP_LOG(INFO, this)
        << "Power override already correctly set, doing nothing";
    return;
  }

  getQsfpFieldAddress(CmisField::MODULE_CONTROL, dataAddress, offset, length);

  uint8_t currentModuleControl;
  getFieldValueLocked(CmisField::MODULE_CONTROL, &currentModuleControl);

  // LowPwr is on the 6 bit of ModuleControl.
  currentModuleControl = currentModuleControl | (1 << 6);

  // first set to low power
  writeCmisField(CmisField::MODULE_CONTROL, &currentModuleControl);

  // Transceivers need a bit of time to handle the low power setting
  // we just sent. We should be able to use the status register to be
  // smarter about this, but just sleeping 0.1s for now.
  usleep(kUsecBetweenPowerModeFlap);

  // then enable target power class
  currentModuleControl = currentModuleControl & 0x3f;

  writeCmisField(CmisField::MODULE_CONTROL, &currentModuleControl);

  QSFP_LOG(INFO, this) << folly::sformat(
      "QSFP module control field set to {:#x}", currentModuleControl);
}

void CmisModule::ensureRxOutputSquelchEnabled(
    const std::vector<HostLaneSettings>& hostLanesSettings) {
  bool allLanesRxOutputSquelchEnabled = true;
  for (auto& hostLaneSettings : hostLanesSettings) {
    if (hostLaneSettings.rxSquelch().has_value() &&
        *hostLaneSettings.rxSquelch()) {
      allLanesRxOutputSquelchEnabled = false;
      break;
    }
  }

  if (!allLanesRxOutputSquelchEnabled) {
    uint8_t enableAllLaneRxOutputSquelch = 0x0;

    writeCmisField(
        CmisField::RX_SQUELCH_DISABLE, &enableAllLaneRxOutputSquelch);
    QSFP_LOG(INFO, this) << "Enabled Rx output squelch on all lanes.";
  }
}

bool CmisModule::tcvrPortStateSupported(TransceiverPortState& portState) const {
  lock_guard<std::mutex> g(qsfpModuleMutex_);
  auto currTransmitterTechnology = getQsfpTransmitterTechnology();
  bool activeElectricalCable = false;
  if (isAecModule()) {
    activeElectricalCable = true;
  }
  if (currTransmitterTechnology == TransmitterTechnology::OPTICAL &&
      (portState.transmitterTech != TransmitterTechnology::OPTICAL &&
       portState.transmitterTech != TransmitterTechnology::BACKPLANE)) {
    // For optics, we allow both BACKPLANE and OPTICAL media in platform mapping
    return false;
  } else if (
      currTransmitterTechnology == TransmitterTechnology::COPPER &&
      !activeElectricalCable) {
    // For Active cables, the tcvr is configrable, so we need to check.
    // For passive copper cables, return true irrespective of speed as the
    // copper cables are mostly flexible with all speeds. We can change this
    // later when we know of any limitations.
    return true;
  }

  auto speed = portState.speed;
  auto startHostLane = portState.startHostLane;
  auto numHostLanes = portState.numHostLanes;
  std::vector<uint8_t> appCodes;
  if (activeElectricalCable) {
    appCodes = CmisHelper::getInterfaceCode(
        speed, CmisHelper::getActiveSpeedApplication());
  } else {
    appCodes = CmisHelper::getInterfaceCode(
        speed, CmisHelper::getSmfSpeedApplicationMapping());
  }
  if (appCodes.empty()) {
    // Speed Not supported
    return false;
  }

  for (auto application : appCodes) {
    if (auto capability = getApplicationField(application, startHostLane)) {
      // Application supported on the starting host lane
      auto hostLaneCount = capability->hostLaneCount;
      if (numHostLanes == hostLaneCount) {
        // Host Lane count also matches
        return true;
      }
    }
  }
  return false;
}

void CmisModule::customizeTransceiverLocked(TransceiverPortState& portState) {
  auto& portName = portState.portName;
  auto speed = portState.speed;
  auto startHostLane = portState.startHostLane;
  auto numHostLanes = portState.numHostLanes;
  QSFP_LOG(INFO, this) << folly::sformat(
      "customizeTransceiverLocked: PortName {}, Speed {}, StartHostLane {}",
      portName,
      apache::thrift::util::enumNameSafe(speed),
      startHostLane);
  /*
   * This must be called with a lock held on qsfpModuleMutex_
   */
  if (customizationSupported()) {
    if (!programAppSelInLowPowerMode()) {
      // We want this on regardless of speed
      setPowerOverrideIfSupportedLocked(
          getPowerControlValue(false /* readFromCache */));
    } else {
      QSFP_LOG(INFO, this)
          << "Module is kept in low power mode for AppSel programming and skipping power override";
    }

    if (isTunableOptics()) {
      if (portState.opticalChannelConfig.has_value()) {
        programTunableModule(portState.opticalChannelConfig.value());
      } else {
        QSFP_LOG(ERR, this) << "Tunable optics requires optical channel config";
      }
    }

    if (speed != cfg::PortSpeed::DEFAULT) {
      setApplicationCodeLocked(
          speed, startHostLane, numHostLanes, kInvalidApplication);
    }

    // For 200G-FR4 module operating in 2x50G mode, disable squelch on all lanes
    // so that each lanes can operate independently
    if (getModuleMediaInterface() == MediaInterfaceCode::FR4_200G &&
        speed == cfg::PortSpeed::FIFTYTHREEPOINTONETWOFIVEG) {
      uint8_t squelchDisableValue = 0xF;
      writeCmisField(CmisField::TX_SQUELCH_DISABLE, &squelchDisableValue);
      writeCmisField(CmisField::RX_SQUELCH_DISABLE, &squelchDisableValue);
      QSFP_LOG(DBG1, this) << "Disabled TX and RX Squelch";
    }
    // Set the FEC sampling if applicable.
    setMaxFecSamplingLocked();
    // Handle vendor-specific low power mode clearing after AppSel programming
    if (programAppSelInLowPowerMode()) {
      const PowerControlState powerState = getCurrentPowerControlState();
      if (powerState == PowerControlState::HIGH_POWER_OVERRIDE &&
          isModuleInReadyState()) {
        QSFP_LOG(INFO, this) << "Module already in high power and ready state";
      } else {
        uint8_t currentModuleControl;
        readCmisField(CmisField::MODULE_CONTROL, &currentModuleControl);
        uint8_t newModuleControl = currentModuleControl & ~LOW_PWR_BIT;
        QSFP_LOG(INFO, this) << folly::sformat(
            "Clearing low power bit to enable high power mode: {:#x} currentModuleControl: {:#x})",
            newModuleControl,
            currentModuleControl);
        writeCmisField(CmisField::MODULE_CONTROL, &newModuleControl);

        getCurrentPowerControlState();

        if (!moduleReadyStatePoll()) {
          QSFP_LOG(ERR, this) << "Module not in ready state";
        }
      }
    }
  } else {
    QSFP_LOG(DBG1, this) << "Customization not supported";
  }
  return;
}

void CmisModule::programTunableModule(
    const cfg::OpticalChannelConfig& opticalChannelConfig) {
  int16_t channelNum = 0;
  int32_t frequencyMhz = 0;
  const auto& freqConfig = opticalChannelConfig.frequencyConfig();
  const auto& centerFreq = freqConfig->centerFrequencyConfig();

  QSFP_LOG(INFO, this) << "Program tunable optics module";

  switch (centerFreq->getType()) {
    case cfg::CenterFrequencyConfig::Type::frequencyMhz: {
      frequencyMhz = centerFreq->frequencyMhz().value();
      channelNum = getChannelNumFromFrequency(
          frequencyMhz, *freqConfig->frequencyGrid());
      break;
    }
    case cfg::CenterFrequencyConfig::Type::channelNumber:
      channelNum = centerFreq->channelNumber().value();
      break;
    default:
      // Handle error case - no field set
      throw FbossError("No field set in CenterFrequencyConfig");
  }

  uint8_t gridSelection =
      frequencyGridToGridSelection(*freqConfig->frequencyGrid());

  // Write grid selection followed by channel number, which is the typical order
  // recommended
  writeCmisField(CmisField::MEDIA_TX_1_GRID_AND_FINE_TUNE_ENA, &gridSelection);
  QSFP_LOG(INFO, this) << folly::sformat(
      "Programmed gridSelection {} on the tunable optics", gridSelection);

  uint8_t channelNumBytes[2];
  channelNumBytes[1] = static_cast<uint8_t>(channelNum & 0XFF);
  channelNumBytes[0] = static_cast<uint8_t>((channelNum >> 8) & 0XFF);

  // Channel number programming
  writeCmisField(CmisField::MEDIA_TX_1_CHAN_NBR_SEL, channelNumBytes);
  QSFP_LOG(INFO, this) << folly::sformat(
      "Programmed Channel number on the tunable optics. frequency {} channel_number {} channelNumBytes[0] {} channelNumBytes[1] {}",
      frequencyMhz,
      channelNum,
      channelNumBytes[0],
      channelNumBytes[1]);
}

uint8_t CmisModule::frequencyGridToGridSelection(FrequencyGrid grid) const {
  uint8_t gridSelection = 0x0;
  switch (grid) {
    case FrequencyGrid::LASER_3P125GHZ:
      gridSelection = 0x00;
      break;
    case FrequencyGrid::LASER_6P25GHZ:
      gridSelection = 0x10;
      break;
    case FrequencyGrid::LASER_12P5GHZ:
      gridSelection = 0x20;
      break;
    case FrequencyGrid::LASER_25GHZ:
      gridSelection = 0x30;
      break;
    case FrequencyGrid::LASER_50GHZ:
      gridSelection = 0x40;
      break;
    case FrequencyGrid::LASER_100GHZ:
      gridSelection = 0x50;
      break;
    case FrequencyGrid::LASER_33GHZ:
      gridSelection = 0x60;
      break;
    case FrequencyGrid::LASER_75GHZ:
      gridSelection = 0x70;
      break;
    case FrequencyGrid::LASER_150GHZ:
      gridSelection = 0x80;
      break;
    default:
      throw FbossError("Invalid FrequencyGrid value: ", static_cast<int>(grid));
  }
  return gridSelection;
}

int16_t CmisModule::getChannelNumFromFrequency(
    int32_t frequencyMhz,
    FrequencyGrid frequencyGrid) {
  int32_t channelNum = 0;
  switch (frequencyGrid) {
    case FrequencyGrid::LASER_150GHZ:
      channelNum =
          (((frequencyMhz - kDefaultFrequencyMhz) * kMhzToGhzFactor * 40) - 3);
      break;
    case FrequencyGrid::LASER_100GHZ:
      channelNum =
          ((frequencyMhz - kDefaultFrequencyMhz) * kMhzToGhzFactor * 10);
      break;
    case FrequencyGrid::LASER_75GHZ:
      channelNum =
          (((frequencyMhz - kDefaultFrequencyMhz) * kMhzToGhzFactor * 40));
      break;
    case FrequencyGrid::LASER_50GHZ:
      channelNum =
          (((frequencyMhz - kDefaultFrequencyMhz) * kMhzToGhzFactor * 20));
      break;
    case FrequencyGrid::LASER_33GHZ:
      channelNum =
          (((frequencyMhz - kDefaultFrequencyMhz) * kMhzToGhzFactor * 30));
      break;
    case FrequencyGrid::LASER_25GHZ:
      channelNum =
          (((frequencyMhz - kDefaultFrequencyMhz) * kMhzToGhzFactor * 40));
      break;
    case FrequencyGrid::LASER_12P5GHZ:
      channelNum =
          (((frequencyMhz - kDefaultFrequencyMhz) * kMhzToGhzFactor * 80));
      break;
    case FrequencyGrid::LASER_6P25GHZ:
      channelNum =
          (((frequencyMhz - kDefaultFrequencyMhz) * kMhzToGhzFactor * 160));
      break;
    case FrequencyGrid::LASER_3P125GHZ:
      channelNum =
          ((frequencyMhz - kDefaultFrequencyMhz) * kMhzToGhzFactor * 320);
      break;
    default:
      throw FbossError(
          "Unsupported frequency grid: ",
          apache::thrift::util::enumNameOrThrow(frequencyGrid));
  }
  return channelNum;
}

/*
 * ensureTransceiverReadyLocked
 *
 * If the current power configuration state is not same as desired one then
 * change it to that (by setting and resetting LP mode) otherwise return true
 * when module is in ready state otherwise return false.
 */
bool CmisModule::ensureTransceiverReadyLocked() {
  // If customization is not supported then the Power control bit can't be
  // touched. Return true as nothing needs to be done here
  if (!customizationSupported()) {
    QSFP_LOG(DBG1, this)
        << "ensureTransceiverReadyLocked: Customization not supported";
    return true;
  }

  // Read the current power configuration values. Don't depend on refresh
  // because that may be delayed
  const PowerControlState powerState = getCurrentPowerControlState();

  // If Optics current power configuration is High Power then the config is
  // correct. We need to check if the Module's current status is READY then
  // return true else return false as the optics state machine might be in
  // transition and need more time to be ready
  if (powerState == PowerControlState::HIGH_POWER_OVERRIDE) {
    return isModuleInReadyState();
  }

  // If the optics current power configuration is Low Power then set the LP
  // mode, wait, reset the LP mode and then return false since the module
  // needs some time to converge its state machine

  // Set to 0x60 = (SquelchControl=Reduce Pave | LowPwr)
  uint8_t newModuleControl = SQUELCH_CONTROL | LOW_PWR_BIT;

  QSFP_LOG(INFO, this) << folly::sformat(
      "ensureTransceiverReadyLocked: Setting module to low power mode with squelch control: {:#x}",
      newModuleControl);

  // first set to low power
  writeCmisField(CmisField::MODULE_CONTROL, &newModuleControl);

  // Wait for 100ms before resetting the LP mode
  /* sleep override */
  usleep(kUsecBetweenPowerModeFlap);

  if (isTunableOptics()) {
    QSFP_LOG(INFO, this) << folly::sformat(
        "Optics is tunable {}", getNameString());
    // Deactivate all the datapath lane before putting into the high power mode
    uint8_t dataPathDeInitReg;
    readCmisField(CmisField::DATA_PATH_DEINIT, &dataPathDeInitReg);
    QSFP_LOG(INFO, this) << folly::sformat(
        "deinit value {} {}", dataPathDeInitReg, getNameString());
    // First deactivate all the lanes
    uint8_t dataPathDeInit = 0xFF;
    writeCmisField(CmisField::DATA_PATH_DEINIT, &dataPathDeInit);
    /* TODO: The generic implementation based on the counter
     * is coming in the diff stack D83613514.
     * The module takes around 2 to 3 seconds to be in dp-deactivated state.
     */
    // Wait for all datapath state machines to get Deactivated
    const auto maxRetriesDeInit = maxRetriesWith500msDelay(/*init=*/false);

    auto retries = 0;
    while (retries++ < maxRetriesDeInit) {
      /* sleep override */
      usleep(kUsecDatapathStatePollTime);
      if (isDatapathUpdated(dataPathDeInit, {CmisLaneState::DEACTIVATED})) {
        break;
      }
    }
    if (retries >= maxRetriesDeInit) {
      QSFP_LOG(ERR, this) << fmt::format(
          "Datapath could not deactivate even after waiting {:d} uSec",
          kUsecDatapathStateUpdateTime);
    }
  }

  // Clear low power bit (set to 0x20)
  if (!programAppSelInLowPowerMode()) {
    newModuleControl = SQUELCH_CONTROL;
    QSFP_LOG(INFO, this) << folly::sformat(
        "ensureTransceiverReadyLocked: Clearing low power bit to enable high power mode: {:#x}",
        newModuleControl);

    writeCmisField(CmisField::MODULE_CONTROL, &newModuleControl);
    return false;
  } else {
    // Maintaining the optics to low power mode until AppSel programming
    // completion
    QSFP_LOG(INFO, this)
        << "ensureTransceiverReadyLocked: Optics in low power mode for AppSel programming";
    return true;
  }
}

/*
 * configureModule
 *
 * Set the module serdes / Rx equalizer after module has been discovered. This
 * is done only if current serdes setting is different from desired one and if
 * the setting is specified in the qsfp config
 */
void CmisModule::configureModule(uint8_t startHostLane) {
  auto moduleTypeEncoding = getMediaTypeEncoding();
  if (moduleTypeEncoding == MediaTypeEncodings::PASSIVE_CU ||
      moduleTypeEncoding == MediaTypeEncodings::ACTIVE_CABLES) {
    // Nothing to configure for passive copper modules
    // TODO: The getModuleConfigOverrideFactor and config expect SMF
    // values. These values dont seem to be applicable to Active Cables,
    // so we will ignore this for now.
    return;
  }

  auto appCode = getSmfMediaInterface(startHostLane);
  auto capability =
      getApplicationField(static_cast<uint8_t>(appCode), startHostLane);

  if (!capability) {
    QSFP_LOG(ERR, this) << "can't find the application capability for "
                        << apache::thrift::util::enumNameSafe(appCode);
    return;
  }
  QSFP_LOG(INFO, this) << "configureModule for application "
                       << apache::thrift::util::enumNameSafe(appCode)
                       << " starting on host lane " << startHostLane;

  auto moduleFactor = getModuleConfigOverrideFactor(
      std::nullopt, // Part Number : TODO: Read and cache tcvrPartNumber
      appCode // Application code
  );

  // Set the Rx equalizer setting based on QSFP config
  for (const auto& override : tcvrConfig_->overridesConfig_) {
    // Check if there is an override for all kinds of transceivers or
    // an override for the current application code(speed)
    if (overrideFactorMatchFound(
            *override.factor(), // override factor
            moduleFactor)) {
      // Check if this override factor requires overriding RxEqualizerSettings
      if (auto rxEqSetting =
              cmisRxEqualizerSettingOverride(*override.config())) {
        setModuleRxEqualizerLocked(
            *rxEqSetting, startHostLane, capability->hostLaneCount);
        return;
      }
    }
  }

  QSFP_LOG(INFO, this)
      << "Rx Equalizer configuration not specified in the QSFP config";
}

bool CmisModule::isTunableOptics() const {
  auto info = QsfpFieldInfo<CmisField, CmisPages>::getQsfpFieldAddress(
      cmisFields, CmisField::MEDIA_INTERFACE_TECHNOLOGY);
  const uint8_t* data =
      getQsfpValuePtr(info.dataAddress, info.offset, info.length);

  uint8_t transTech = *data;
  if ((transTech == DeviceTechnologyCmis::C_BAND_TUNABLE_LASER_CMIS) ||
      (transTech == DeviceTechnologyCmis::L_BAND_TUNABLE_LASER_CMIS)) {
    return true;
  } else {
    return false;
  }
}

bool CmisModule::isLpoModule() const {
  // FACETESTLPO is for testing purposes.
  Vendor vendor = getVendorInfo();
  if (vendor.name().value() == "FACETESTLPO") {
    return true;
  }
  // Expected host/media interface codes for LPO transceivers.
  std::set<std::pair<ActiveCuHostInterfaceCode, SMFMediaInterfaceCode>>
      expectedLpoInterfaces = {
          {ActiveCuHostInterfaceCode::LPO_100G,
           SMFMediaInterfaceCode::FR1_100G},
          {ActiveCuHostInterfaceCode::LPO_400G,
           SMFMediaInterfaceCode::FR4_400G},
          {ActiveCuHostInterfaceCode::LPO_800G,
           SMFMediaInterfaceCode::FR8_800G}};

  if (moduleCapabilities_.size() == expectedLpoInterfaces.size()) {
    for (const auto& capability : moduleCapabilities_) {
      if (!expectedLpoInterfaces.erase(
              std::make_pair(
                  static_cast<ActiveCuHostInterfaceCode>(
                      capability.moduleHostInterface),
                  static_cast<SMFMediaInterfaceCode>(
                      capability.moduleMediaInterface)))) {
        break;
      }
    }
    if (expectedLpoInterfaces.empty()) {
      return true;
    }
  }
  return false;
}

MediaInterfaceCode CmisModule::getModuleMediaInterface() const {
  // Return the MediaInterfaceCode based on the first application
  auto moduleMediaInterface = MediaInterfaceCode::UNKNOWN;
  auto mediaTypeEncoding = getMediaTypeEncoding();
  if (mediaTypeEncoding == MediaTypeEncodings::PASSIVE_CU) {
    // FIXME: Remove CR8_400G hardcoding and derive this from number of
    // lanes/host electrical interface instead
    moduleMediaInterface = MediaInterfaceCode::CR8_400G;
  } else if (
      mediaTypeEncoding == MediaTypeEncodings::OPTICAL_SMF &&
      moduleCapabilities_.size() > 0) {
    auto firstModuleCapability = moduleCapabilities_.begin();
    auto smfCode = static_cast<SMFMediaInterfaceCode>(
        firstModuleCapability->moduleMediaInterface);
    if (isLpoModule()) {
      moduleMediaInterface = MediaInterfaceCode::FR4_LPO_2x400G;
    } else if (
        smfCode == SMFMediaInterfaceCode::FR4_400G &&
        firstModuleCapability->hostStartLanes.size() == 2) {
      if (getQsfpSMFLength() == kFR4LiteSMFLength) {
        // Lite Modules are not LPO modules but have a reach of 500m.
        moduleMediaInterface = MediaInterfaceCode::FR4_LITE_2x400G;
      } else {
        moduleMediaInterface = MediaInterfaceCode::FR4_2x400G;
      }
    } else if (
        smfCode == SMFMediaInterfaceCode::DR4_400G &&
        firstModuleCapability->hostStartLanes.size() == 2) {
      moduleMediaInterface = MediaInterfaceCode::DR4_2x400G;
    } else if (
        smfCode == SMFMediaInterfaceCode::LR4_10_400G &&
        firstModuleCapability->hostStartLanes.size() == 2) {
      moduleMediaInterface = MediaInterfaceCode::LR4_2x400G_10KM;
    } else if (
        smfCode == SMFMediaInterfaceCode::DR4_800G &&
        firstModuleCapability->hostStartLanes.size() == 2) {
      moduleMediaInterface = MediaInterfaceCode::DR4_2x800G;
    } else {
      moduleMediaInterface =
          CmisHelper::getMediaInterfaceCode<SMFMediaInterfaceCode>(
              smfCode, CmisHelper::getSmfMediaInterfaceMapping());
    }
  } else if (mediaTypeEncoding == MediaTypeEncodings::ACTIVE_CABLES) {
    // TODO: For now, we only support the 8x100G Active Electrical Cable.
    // We can use the number of lanes and Interface codes to derive
    // the MediaInterfaceCode.
    moduleMediaInterface = MediaInterfaceCode::CR8_800G;
  }

  return moduleMediaInterface;
};

/*
 * setModuleRxEqualizerLocked
 *
 * Customize the optics for Rx pre-cursor, Rx post cursor, Rx main amplitude
 * 1. Check P11h, B223-234 to compare current and desired pre/post/main
 * 2. Write P10h, B162-165 for Pre
 * 3. Write P10h, B166-169 for Post
 * 4. Write P10h, B170-173 for Main
 * 5. Write P10h, B145-152 bit 0 = 1 to use Staged Set 0 values
 * 6. Write P10h, B144 = 0xFF to apply stage 0 control value immediately
 */
void CmisModule::setModuleRxEqualizerLocked(
    RxEqualizerSettings rxEqualizer,
    uint8_t startHostLane,
    uint8_t hostLaneCount) {
  uint8_t currPre[4], currPost[4], currMain[4];
  // Read the existing settings to compare with desired settings later
  readCmisField(CmisField::RX_OUT_PRE_CURSOR, currPre);
  readCmisField(CmisField::RX_OUT_POST_CURSOR, currPost);
  readCmisField(CmisField::RX_OUT_MAIN, currMain);

  uint8_t desiredPre[4], desiredPost[4], desiredMain[4];
  // Initialize desired settings with the current settings
  for (int i = 0; i < 4; i++) {
    desiredPre[i] = currPre[i];
    desiredPost[i] = currPost[i];
    desiredMain[i] = currMain[i];
  }
  bool changePre = false, changePost = false, changeMain = false;

  QSFP_LOG(INFO, this) << "setModuleRxEqualizerLocked called with startLane = "
                       << startHostLane
                       << ", hostLaneCount = " << hostLaneCount;

  // Update the desired settings for the relevant lanes
  for (auto lane = startHostLane; lane <= (startHostLane + hostLaneCount - 1);
       lane++) {
    // Two lanes share the same byte. offsetIndex tracks which of the 4 bytes
    // corresponds to lane
    int offsetIndex = lane / 2;
    // For odd lanes, values are at the upper 4 bits. shiftOffset will be 4 for
    // odd lanes, 0 for even lanes
    int shiftOffset = 0;
    if (lane % 2) {
      shiftOffset = 4;
    }
    // Clear the bits so that we can set them next
    desiredPre[offsetIndex] &= ~(0xf << shiftOffset);
    desiredPost[offsetIndex] &= ~(0xf << shiftOffset);
    desiredMain[offsetIndex] &= ~(0xf << shiftOffset);

    // Set the settings for the corresponding lane
    desiredPre[offsetIndex] |=
        ((*rxEqualizer.preCursor() & 0xf) << shiftOffset);
    desiredPost[offsetIndex] |=
        ((*rxEqualizer.postCursor() & 0xf) << shiftOffset);
    desiredMain[offsetIndex] |=
        ((*rxEqualizer.mainAmplitude() & 0xf) << shiftOffset);
  }

  auto compareSettings = [startHostLane, hostLaneCount](
                             uint8_t currSettings[],
                             uint8_t desiredSettings[],
                             int length,
                             bool& changeNeeded) {
    // Two lanes share the same byte so loop only until numLanes / 2
    for (auto i = startHostLane / 2;
         i <= (startHostLane + hostLaneCount - 1) / 2;
         i++) {
      if (i < length && currSettings[i] != desiredSettings[i]) {
        // Some of the pre-cursor value needs to be changed so break from
        // here
        changeNeeded = true;
        return;
      }
    }
  };

  // Compare current Pre cursor value to see if the change is needed
  compareSettings(currPre, desiredPre, 4, changePre);
  // Compare current Post cursor value to see if the change is needed
  compareSettings(currPost, desiredPost, 4, changePost);
  // Compare current Rx Main value to see if the change is needed
  compareSettings(currMain, desiredMain, 4, changeMain);

  // If anything is changed then apply the change and trigger it
  if (changePre || changePost || changeMain) {
    for (auto i = startHostLane / 2;
         i <= (startHostLane + hostLaneCount - 1) / 2;
         i++) {
      // Apply the change for pre/post/main if needed
      if (changePre) {
        writeCmisField(offsetIndexToCmisField[i][0], &desiredPre[i]);
        QSFP_LOG(INFO, this) << folly::sformat(
            "customized index {:d} for Pre-cursor 0x{:x}", i, desiredPre[i]);
      }
      if (changePost) {
        writeCmisField(offsetIndexToCmisField[i][1], &desiredPost[i]);
        QSFP_LOG(INFO, this) << folly::sformat(
            "customized index {:d} for Post-cursor 0x{:x}", i, desiredPost[i]);
      }
      if (changeMain) {
        writeCmisField(offsetIndexToCmisField[i][2], &desiredMain[i]);
        QSFP_LOG(INFO, this) << folly::sformat(
            "customized index {:d} for Rx-out-main 0x{:x}", i, desiredMain[i]);
      }
    }

    // Apply the change using stage 0 control
    uint8_t stage0Control[8];
    readCmisField(CmisField::APP_SEL_LANE_1_8, stage0Control);
    std::set<uint8_t> lanesToConfigure;
    std::vector<uint8_t> stageControlToWrite;
    for (int i = startHostLane; i < startHostLane + hostLaneCount; i++) {
      stage0Control[i] |= 1;
      lanesToConfigure.insert(i);
      stageControlToWrite.push_back(stage0Control[i]);
    }
    writeCmisField(
        laneToAppSelField(lanesToConfigure),
        stageControlToWrite.data(),
        true /* skipPageChange */);

    // Trigger the stage 0 control values to be operational in optics
    uint8_t stage0ControlTrigger = laneMask(startHostLane, hostLaneCount);
    writeCmisField(CmisField::STAGE_CTRL_SET0_IMMEDIATE, &stage0ControlTrigger);

    // Check if the config has been applied correctly or not
    if (!checkLaneConfigError(startHostLane, hostLaneCount)) {
      QSFP_LOG(ERR, this) << "customization config rejected";
    }
  }
}

/*
 * setDiagsCapability
 *
 * This function reads the module register from cache and populates the
 * diagnostic capability. This function is called from Module State Machine
 * when the MSM enters Module Discovered state after EEPROM read.
 */
void CmisModule::setDiagsCapability() {
  if (flatMem_) {
    // Upper pages > 0 are not applicable for flatMem_ modules, and hence
    // diagsCapability isn't valid either
    return;
  }
  // Limiting the scope of diagsCapability_ write lock
  {
    auto diagsCapability = diagsCapability_.wlock();
    if (!diagsCapability->has_value()) {
      QSFP_LOG(INFO, this) << "Setting diag capability";
      DiagsCapability diags;

      auto getPrbsCapabilities =
          [&](CmisField generatorField,
              CmisField checkerField) -> std::vector<prbs::PrbsPolynomial> {
        int offset;
        int length;
        int dataAddress;
        getQsfpFieldAddress(generatorField, dataAddress, offset, length);
        CHECK_EQ(length, 2);
        getQsfpFieldAddress(checkerField, dataAddress, offset, length);
        CHECK_EQ(length, 2);

        uint8_t generatorCapsData[2];
        readFromCacheOrHw(generatorField, generatorCapsData);
        uint16_t generatorCaps =
            (generatorCapsData[1] << 8) | generatorCapsData[0];

        uint8_t checkerCapsData[2];
        readFromCacheOrHw(checkerField, checkerCapsData);
        uint16_t checkerCaps = (checkerCapsData[1] << 8) | checkerCapsData[0];

        std::vector<prbs::PrbsPolynomial> caps;
        for (auto patternIDPolynomialPair : prbsPatternMap.right) {
          // We claim PRBS polynomial is supported when both generator and
          // checker support the polynomial
          if (generatorCaps & (1 << patternIDPolynomialPair.first) &&
              checkerCaps & (1 << patternIDPolynomialPair.first)) {
            caps.push_back(patternIDPolynomialPair.second);
          }
        }
        return caps;
      };

      uint8_t data;
      readFromCacheOrHw(CmisField::VDM_DIAG_SUPPORT, &data);
      diags.vdm() = (data & FieldMasks::VDM_SUPPORT_MASK) ? true : false;
      diags.diagnostics() =
          (data & FieldMasks::DIAGS_SUPPORT_MASK) ? true : false;

      readFromCacheOrHw(CmisField::CDB_SUPPORT, &data);
      diags.cdb() = (data & FieldMasks::CDB_SUPPORT_MASK) ? true : false;

      readFromCacheOrHw(CmisField::TX_CONTROL_SUPPORT, &data);
      diags.txOutputControl() =
          (data & FieldMasks::TX_DISABLE_SUPPORT_MASK) ? true : false;
      readFromCacheOrHw(CmisField::RX_CONTROL_SUPPORT, &data);
      diags.rxOutputControl() =
          (data & FieldMasks::RX_DISABLE_SUPPORT_MASK) ? true : false;

      if (*diags.diagnostics()) {
        readFromCacheOrHw(CmisField::LOOPBACK_CAPABILITY, &data);
        diags.loopbackSystem() =
            (data & FieldMasks::LOOPBACK_SYS_SUPPOR_MASK) ? true : false;
        diags.loopbackLine() =
            (data & FieldMasks::LOOPBACK_LINE_SUPPORT_MASK) ? true : false;

        readFromCacheOrHw(CmisField::PATTERN_CHECKER_CAPABILITY, &data);
        diags.prbsLine() =
            (data & FieldMasks::PRBS_LINE_SUPPRT_MASK) ? true : false;
        diags.prbsSystem() =
            (data & FieldMasks::PRBS_SYS_SUPPRT_MASK) ? true : false;
        if (*diags.prbsLine()) {
          diags.prbsLineCapabilities() = getPrbsCapabilities(
              CmisField::MEDIA_SUPPORTED_GENERATOR_PATTERNS,
              CmisField::MEDIA_SUPPORTED_CHECKER_PATTERNS);
        }
        if (*diags.prbsSystem()) {
          diags.prbsSystemCapabilities() = getPrbsCapabilities(
              CmisField::HOST_SUPPORTED_GENERATOR_PATTERNS,
              CmisField::HOST_SUPPORTED_CHECKER_PATTERNS);
        }

        readFromCacheOrHw(CmisField::DIAGNOSTIC_CAPABILITY, &data);
        diags.snrLine() = data & FieldMasks::SNR_LINE_SUPPORT_MASK;
        diags.snrSystem() = data & FieldMasks::SNR_SYS_SUPPORT_MASK;
      }

      if (*diags.vdm()) {
        readCmisField(CmisField::VDM_GROUPS_SUPPORT, &data);
        vdmSupportedGroupsMax_ = (data & VDM_GROUPS_SUPPORT_MASK) + 1;
      }

      *diagsCapability = diags;
    }
  }
  // Scan and update the VDM diags locations
  updateVdmDiagsValLocation();
}

/*
 * verifyEepromChecksums
 *
 * This function verifies the module's eeprom register checksum in various
 * pages. For CMIS module the checksums are kept in 3 pages:
 *   Page 0: Register 222 contains checksum for values in register 128 to 221
 *   Page 1: Register 255 contains checksum for values in register 130 to 254
 *   Page 2: Register 255 contains checksum for values in register 128 to 254
 * These checksums are 8 bit sum of all the 8 bit values
 */
bool CmisModule::verifyEepromChecksums() {
  bool rc = true;
  // Verify checksum for all pages
  for (auto& csumInfoIt : checksumInfoCmis) {
    // For flat memory module, check for page 0 only
    if (flatMem_ && csumInfoIt.first != CmisPages::PAGE00) {
      continue;
    }
    rc &= verifyEepromChecksum(csumInfoIt.first);
  }
  QSFP_LOG_IF(WARN, !rc, this) << "EEPROM Checksum Failed";
  return rc;
}

/*
 * verifyEepromChecksum
 *
 * This function verifies the module's eeprom register checksum for a given
 * page. The checksum is 8 bit sum of all the 8 bit values in range of
 * registers
 */
bool CmisModule::verifyEepromChecksum(CmisPages pageId) {
  int offset;
  int length;
  int dataAddress;
  const uint8_t* data;
  uint8_t checkSum = 0, expectedChecksum;

  // Return false if the registers are not cached yet (this is not expected)
  if (!cacheIsValid()) {
    QSFP_LOG(WARN, this)
        << "can't do eeprom checksum as the register cache is not populated";
    return false;
  }
  // Return false if we don't know range of registers to validate the checksum
  // on this page
  if (checksumInfoCmis.find(pageId) == checksumInfoCmis.end()) {
    QSFP_LOG(WARN, this) << "can't do eeprom checksum for page "
                         << static_cast<int>(pageId);
    return false;
  }

  // Get the range of registers, compute checksum and compare
  dataAddress = static_cast<int>(pageId);
  offset = checksumInfoCmis[pageId].checksumRangeStartOffset;
  length = checksumInfoCmis[pageId].checksumRangeLength;
  data = getQsfpValuePtr(dataAddress, offset, length);

  for (int i = 0; i < length; i++) {
    checkSum += data[i];
  }

  getQsfpFieldAddress(
      checksumInfoCmis[pageId].checksumValOffset, dataAddress, offset, length);
  data = getQsfpValuePtr(dataAddress, offset, length);
  expectedChecksum = data[0];

  if (checkSum != expectedChecksum) {
    QSFP_LOG(ERR, this) << folly::sformat(
        "Page {:d}: expected eeprom checksum {:#x}, actual {:#x}",
        static_cast<int>(pageId),
        expectedChecksum,
        checkSum);
    return false;
  } else {
    QSFP_LOG(DBG5, this) << folly::sformat(
        "Page {:d}: eeprom checksum verified successfully {:#x}",
        static_cast<int>(pageId),
        checkSum);
  }
  return true;
}

/*
 * latchAndReadVdmDataLocked
 *
 * This function holds the latch and reads the VDM data from the module. This
 * function call will be triggered by StatsPublisher thread setting the atomic
 * variable to capture the VDM stats (typically every 5 minutes)
 */
void CmisModule::latchAndReadVdmDataLocked() {
  if (!isVdmSupported()) {
    return;
  }
  QSFP_LOG(DBG3, this) << "latchAndReadVdmDataLocked";

  // Write 2F.144 bit 7 to 1 (hold latch, pause counters)
  uint8_t latchRequest;
  readCmisField(CmisField::VDM_LATCH_REQUEST, &latchRequest);

  latchRequest |= FieldMasks::VDM_LATCH_REQUEST_MASK;
  // Hold the latch to read the VDM data
  writeCmisField(CmisField::VDM_LATCH_REQUEST, &latchRequest);

  // Wait tNack time
  /* sleep override */
  usleep(kUsecVdmLatchHold);

  // Read data for publishing to ODS
  readCmisField(CmisField::PAGE_UPPER24H, page24_);
  readCmisField(CmisField::PAGE_UPPER25H, page25_);
  if (isVdmSupported(3)) {
    // Cache VDM group 3 page only if it is supported
    readCmisField(CmisField::PAGE_UPPER26H, page26_);
  }

  // Write Byte 2F.144, bit 7 to 0 (clear latch)
  latchRequest &= ~FieldMasks::VDM_LATCH_REQUEST_MASK;
  // Release the latch to resume VDM data collection. This automatically
  // starts a new VDM interval in HW
  writeCmisField(CmisField::VDM_LATCH_REQUEST, &latchRequest);
  // Wait tNack time
  /* sleep override */
  usleep(kUsecVdmLatchHold);
  vdmIntervalStartTime_ = std::time(nullptr);
}

/*
 * triggerVdmStatsCapture
 *
 * This function triggers the next VDM stats capture by qsfp_service refresh
 * thread. This function gets called by ODS cycle (every 5 mins)
 */
void CmisModule::triggerVdmStatsCapture() {
  if (!isVdmSupported()) {
    return;
  }
  QSFP_LOG(DBG3, this) << "triggerVdmStatsCapture";

  captureVdmStats_ = true;
}

bool CmisModule::getModuleStateChanged() {
  return getSettingsValue(CmisField::MODULE_FLAG, MODULE_STATE_CHANGED_MASK);
}

void CmisModule::clearTransceiverPrbsStats(
    const std::string& portName,
    phy::Side side) {
  auto clearTransceiverPrbsStatsLambda = [side, portName, this]() {
    lock_guard<std::mutex> g(qsfpModuleMutex_);
    // Read modify write
    // Write bit 5 in 13h.177 to 1 and then 0 to reset counters
    uint8_t val;
    readCmisField(CmisField::BER_CTRL, &val);
    val |= BER_CTRL_RESET_STAT_MASK;
    writeCmisField(CmisField::BER_CTRL, &val);
    val &= ~BER_CTRL_RESET_STAT_MASK;
    writeCmisField(CmisField::BER_CTRL, &val);
  };
  auto i2cEvb = qsfpImpl_->getI2cEventBase();
  if (!i2cEvb) {
    // Certain platforms cannot execute multiple I2C transactions in parallel
    // and therefore don't have an I2C evb thread
    clearTransceiverPrbsStatsLambda();
  } else {
    via(i2cEvb)
        .thenValue([clearTransceiverPrbsStatsLambda](auto&&) mutable {
          clearTransceiverPrbsStatsLambda();
        })
        .get();
  }

  // Call the base class implementation to clear the common stats
  QsfpModule::clearTransceiverPrbsStats(portName, side);
}

/*
 * setPortPrbsLocked
 *
 * This function starts or stops the PRBS generator and checker on a given
 * side of optics (line side or host side). The PRBS is supported on new 200G
 * and 400G CMIS optics. This function expects the caller to hold the qsfp
 * module level lock
 */
bool CmisModule::setPortPrbsLocked(
    const std::string& portName,
    phy::Side side,
    const prbs::InterfacePrbsState& prbs) {
  // If PRBS is not supported then return
  if (!isPrbsSupported(side)) {
    QSFP_LOG(ERR, this) << folly::sformat(
        "PRBS not supported on {:s} side",
        (side == phy::Side::LINE ? "Line" : "System"));
    return false;
  }

  // Return error for invalid PRBS polynominal
  auto prbsPatternItr = prbsPatternMap.left.find(
      static_cast<prbs::PrbsPolynomial>(*prbs.polynomial()));

  // Get the list of lanes to enable/disable PRBS
  auto tcvrLanes = getTcvrLanesForPort(portName, side);
  if (tcvrLanes.empty()) {
    QSFP_LOG(ERR, this) << fmt::format(
        "Empty lane list for port {:s}", portName);
    return false;
  }

  bool startGen{false}, stopGen{false};
  bool startChk{false}, stopChk{false};
  if (!prbs.generatorEnabled().has_value() &&
      !prbs.checkerEnabled().has_value()) {
    QSFP_LOG(ERR, this) << "Invalid generator/checker input";
    return false;
  }

  if (prbs.generatorEnabled().has_value()) {
    startGen = prbs.generatorEnabled().value();
    stopGen = !prbs.generatorEnabled().value();
  }
  if (prbs.checkerEnabled().has_value()) {
    startChk = prbs.checkerEnabled().value();
    stopChk = !prbs.checkerEnabled().value();
  }

  // Step 1: Set the pattern for Generator (for starting case)
  if (startGen) {
    auto cmisRegisters = (side == phy::Side::LINE) ? prbsGenMediaPatternFields
                                                   : prbsGenHostPatternFields;

    // Check that a valid polynomial is provided
    if (prbsPatternItr == prbsPatternMap.left.end()) {
      QSFP_LOG(ERR, this) << folly::sformat(
          "PRBS Polynominal {} not supported",
          apache::thrift::util::enumNameSafe(prbs.polynomial().value()));
      return false;
    }
    auto prbsPolynominal = prbsPatternItr->second;
    // There are 4 bytes, each contains pattern for 2 lanes
    uint8_t patternVal;
    for (auto lane : tcvrLanes) {
      auto cmisReg = cmisRegisters[lane / 2];
      readCmisField(cmisReg, &patternVal);
      patternVal = (lane % 2 == 0)
          ? (patternVal & 0xF0) | (prbsPolynominal & 0x0F)
          : (patternVal & 0x0F) | ((prbsPolynominal << 4) & 0xF0);
      writeCmisField(cmisReg, &patternVal);
    }
  }

  // Step 2: Start/Stop the generator
  // Get the bitmask for Start/Stop of generator/checker on the given side
  auto cmisRegister = (side == phy::Side::LINE) ? CmisField::MEDIA_GEN_ENABLE
                                                : CmisField::HOST_GEN_ENABLE;

  if (startGen || stopGen) {
    uint8_t startGenLaneMask;
    readCmisField(cmisRegister, &startGenLaneMask);
    for (auto lane : tcvrLanes) {
      if (startGen) {
        startGenLaneMask |= (1 << lane);
      } else {
        startGenLaneMask &= ~(1 << lane);
      }
    }
    writeCmisField(cmisRegister, &startGenLaneMask);

    QSFP_LOG(INFO, this) << folly::sformat(
        "PRBS Generator on side {:s} Lanemask {:#x} {:s}",
        ((side == phy::Side::LINE) ? "Line" : "Host"),
        startGenLaneMask,
        (startGen ? "Started" : "Stopped"));
  }

  // Step 3: Set the pattern for Checker (for starting case)
  if (startChk) {
    auto& fields = (side == phy::Side::LINE) ? prbsChkMediaPatternFields
                                             : prbsChkHostPatternFields;

    // Check that a valid polynomial is provided
    if (prbsPatternItr == prbsPatternMap.left.end()) {
      QSFP_LOG(ERR, this) << folly::sformat(
          "PRBS Polynominal {} not supported",
          apache::thrift::util::enumNameSafe(prbs.polynomial().value()));
      return false;
    }
    auto prbsPolynominal = prbsPatternItr->second;
    // There are 4 bytes, each contains pattern for 2 lanes
    uint8_t patternVal;
    for (auto lane : tcvrLanes) {
      auto cmisReg = fields[lane / 2];
      readCmisField(cmisReg, &patternVal);
      patternVal = (lane % 2 == 0)
          ? (patternVal & 0xF0) | (prbsPolynominal & 0x0F)
          : (patternVal & 0x0F) | ((prbsPolynominal << 4) & 0xF0);
      writeCmisField(cmisReg, &patternVal);
    }
  }

  // Step 4: Start/Stop the checker
  cmisRegister = (side == phy::Side::LINE) ? CmisField::MEDIA_CHECKER_ENABLE
                                           : CmisField::HOST_CHECKER_ENABLE;

  if (startChk || stopChk) {
    uint8_t startChkLaneMask;
    readCmisField(cmisRegister, &startChkLaneMask);
    for (auto lane : tcvrLanes) {
      if (startChk) {
        startChkLaneMask |= (1 << lane);
      } else {
        startChkLaneMask &= ~(1 << lane);
      }
    }
    writeCmisField(cmisRegister, &startChkLaneMask);

    QSFP_LOG(INFO, this) << folly::sformat(
        "PRBS Checker on side {:s} Lanemask {:#x} {:s}",
        ((side == phy::Side::LINE) ? "Line" : "Host"),
        startChkLaneMask,
        (startChk ? "Started" : "Stopped"));
  }

  return true;
}

// This function expects caller to hold the qsfp module level lock
prbs::InterfacePrbsState CmisModule::getPortPrbsStateLocked(
    std::optional<const std::string> portName,
    Side side) {
  if (flatMem_) {
    return prbs::InterfacePrbsState();
  }
  {
    if (!isTransceiverFeatureSupported(TransceiverFeature::PRBS, side)) {
      return prbs::InterfacePrbsState();
    }
  }
  prbs::InterfacePrbsState state;

  // Get the list of lanes to check PRBS
  uint8_t laneMask = 0;
  if (portName.has_value()) {
    auto tcvrLanes = getTcvrLanesForPort(portName.value(), side);
    if (tcvrLanes.empty()) {
      QSFP_LOG(ERR, this) << fmt::format(
          "Empty lane list for port {:s}", portName.value());
      return prbs::InterfacePrbsState();
    }
    for (auto lane : tcvrLanes) {
      laneMask |= (1 << lane);
    }
  } else {
    laneMask = (side == phy::Side::LINE) ? ((1 << numMediaLanes()) - 1)
                                         : ((1 << numHostLanes()) - 1);
  }
  if (!laneMask) {
    QSFP_LOG(ERR, this) << fmt::format(
        "Lanes not available for getPortPrbsState {:s}", qsfpImpl_->getName());
    return prbs::InterfacePrbsState();
  }

  auto cmisRegister = (side == phy::Side::LINE) ? CmisField::MEDIA_GEN_ENABLE
                                                : CmisField::HOST_GEN_ENABLE;
  uint8_t generator;
  readCmisField(cmisRegister, &generator);

  cmisRegister = (side == phy::Side::LINE) ? CmisField::MEDIA_CHECKER_ENABLE
                                           : CmisField::HOST_CHECKER_ENABLE;
  uint8_t checker;
  readCmisField(cmisRegister, &checker);

  state.generatorEnabled() = (generator & laneMask);
  state.checkerEnabled() = (checker & laneMask);
  // PRBS is enabled if either generator is enabled or the checker is enabled
  auto enabled =
      state.generatorEnabled().value() || state.checkerEnabled().value();

  // If state is enabled, check the polynomial
  if (enabled) {
    std::array<CmisField, 4> cmisPatternRegister = (side == phy::Side::LINE)
        ? prbsGenMediaPatternFields
        : prbsGenHostPatternFields;

    int firstLane = 0, tempLaneMask = laneMask;
    while (tempLaneMask) {
      if (tempLaneMask & 0x1) {
        break;
      }
      firstLane++;
      tempLaneMask >>= 1;
    }

    uint8_t patternByte, pattern;
    // Intentionally reading only 1 byte instead of 'length'
    // We assume the same polynomial is configured on all lanes so only
    // reading 1 byte which gives the polynomial configured on lane 0
    cmisRegister = cmisPatternRegister[firstLane / 2];
    readCmisField(cmisRegister, &patternByte);
    pattern = (patternByte >> (((firstLane % 2) * 4))) & 0xF;
    auto polynomialItr = prbsPatternMap.right.find(pattern);
    if (polynomialItr != prbsPatternMap.right.end()) {
      state.polynomial() = prbs::PrbsPolynomial(polynomialItr->second);
    }
  }
  return state;
}

/*
 * getPortPrbsStatsSideLocked
 *
 * This function retrieves the PRBS stats for all the lanes in a module for
 * the given side of optics (line side or host side). The PRBS checker lock
 * and BER stats are returned.
 */
phy::PrbsStats CmisModule::getPortPrbsStatsSideLocked(
    phy::Side side,
    bool checkerEnabled,
    const phy::PrbsStats& lastStats) {
  phy::PrbsStats prbsStats;

  // If PRBS is not supported then return
  if (!isPrbsSupported(side)) {
    QSFP_LOG(ERR, this) << folly::sformat(
        "PRBS not supported on {:s} side",
        (side == phy::Side::LINE ? "Line" : "System"));
    return phy::PrbsStats{};
  }

  prbsStats.portId() = getID();
  prbsStats.component() = side == Side::SYSTEM
      ? phy::PortComponent::TRANSCEIVER_SYSTEM
      : phy::PortComponent::TRANSCEIVER_LINE;

  if (!checkerEnabled || !lastStats.laneStats()->size()) {
    // If the checker is not enabled or the stats are uninitialized, return
    // the default PrbsStats object with some of the parameters initialized
    int lanes = side == Side::SYSTEM ? numHostLanes() : numMediaLanes();
    for (int lane = 0; lane < lanes; lane++) {
      phy::PrbsLaneStats laneStat;
      laneStat.laneId() = lane;
      prbsStats.laneStats()->push_back(laneStat);
    }
    return prbsStats;
  }

  // Step1: Get the lane locked mask for PRBS checker
  uint8_t checkerLockMask;
  auto cmisRegister = (side == phy::Side::LINE)
      ? CmisField::MEDIA_LANE_CHECKER_LOL_LATCH
      : CmisField::HOST_LANE_CHECKER_LOL_LATCH;
  readCmisField(cmisRegister, &checkerLockMask);

  // Step 2: Get BER values for all the lanes
  int numLanes = (side == phy::Side::LINE) ? numMediaLanes() : numHostLanes();

  // Step 2.a: Set the Diag Sel register to collect the BER values and wait
  // for some time
  uint8_t diagSel =
      DiagnosticFeatureEncoding::BER; // Diag Sel 1 is to obtain BER values
  writeCmisField(CmisField::DIAG_SEL, &diagSel);
  /* sleep override */
  usleep(kUsecDiagSelectLatchWait);

  // Step 2.b: Read the BER values for all lanes
  cmisRegister = (side == phy::Side::LINE) ? CmisField::MEDIA_BER_HOST_SNR
                                           : CmisField::HOST_BER;
  std::array<uint8_t, 16> laneBerList;
  readCmisField(cmisRegister, laneBerList.data());

  // Get the SNR values
  std::array<uint8_t, 16> laneSnrList;
  if (isSnrSupported(side)) {
    // Step 2.c: Set the Diag Sel register to collect the SNR values and wait
    // for some time
    diagSel =
        DiagnosticFeatureEncoding::SNR; // Diag Sel 6 is to obtain SNR values
    writeCmisField(CmisField::DIAG_SEL, &diagSel);
    /* sleep override */
    usleep(kUsecDiagSelectLatchWait);

    // Step 2.d: Read the SNR values for all lanes
    cmisRegister = (side == phy::Side::LINE) ? CmisField::MEDIA_SNR
                                             : CmisField::MEDIA_BER_HOST_SNR;

    readCmisField(cmisRegister, laneSnrList.data());
  }

  // Step 3: Put all the lane info in return structure and return
  for (auto laneId = 0; laneId < numLanes; laneId++) {
    phy::PrbsLaneStats laneStats;
    laneStats.laneId() = laneId;
    // Put the lock value
    laneStats.locked() = (checkerLockMask & (1 << laneId)) == 0;

    // Put the BER value
    uint8_t lsb, msb;
    lsb = laneBerList.at(laneId * 2);
    msb = laneBerList.at((laneId * 2) + 1);
    laneStats.ber() = QsfpModule::getBerFloatValue(lsb, msb);

    // Put the SNR value
    if (isSnrSupported(side)) {
      lsb = laneSnrList.at(laneId * 2);
      msb = laneSnrList.at((laneId * 2) + 1);
      uint16_t snrRawVal = (msb << 8) | lsb;
      laneStats.snr() = CmisFieldInfo::getSnr(snrRawVal);
    }
    laneStats.timeCollected() = std::time(nullptr);
    prbsStats.laneStats()->push_back(laneStats);
  }
  prbsStats.timeCollected() = std::time(nullptr);
  return prbsStats;
}

uint64_t CmisModule::maxRetriesWith500msDelay(bool init) {
  if (isAecModule()) {
    // Read the datapath init/deinit max time from module.
    uint8_t specVal;
    readCmisField(CmisField::MAX_DPINIT_TIME, &specVal);
    // MAX_DP_DEINIT_TIME: bits 7-4
    // MAX_DP_INIT_TIME: bits 3-0
    uint8_t spec = 0;
    if (init) {
      spec = specVal & DP_INIT_MAX_MASK;
    } else {
      spec = specVal & DP_DINIT_MAX_MASK;
      spec >>= DP_DINIT_BITSHIFT;
    }
    auto itr = DpInitValToTimeMap.find(spec);
    if (itr != DpInitValToTimeMap.end()) {
      uint64_t maxTime = itr->second;
      QSFP_LOG(INFO, this) << fmt::format(
          "Datapath max {:s} time from spec is {:d} uSec",
          init ? "init" : "deinit",
          maxTime);
      if (kUsecDatapathStateUpdateTimeMaxFboss > maxTime) {
        // success.
        return maxTime / kUsecDatapathStatePollTime;
      } else {
        QSFP_LOG(ERR, this) << fmt::format(
            "Datapath max {:s} time from spec {:d} uSec is greater than max allowed time {:d} uSec",
            init ? "init" : "deinit",
            maxTime,
            kUsecDatapathStateUpdateTimeMaxFboss);
      }
    } else {
      QSFP_LOG(ERR, this) << fmt::format(
          "Datapath max {:s} time unable to retrieve from val map spec {:x}",
          init ? "init" : "deinit",
          spec);
    }
  }
  QSFP_LOG(INFO, this) << fmt::format(
      "Default max Init/DeInit time {:d} uSec", kUsecDatapathStateUpdateTime);
  return kUsecDatapathStateUpdateTime / kUsecDatapathStatePollTime;
}

bool CmisModule::isDatapathUpdated(
    uint8_t laneMask,
    const std::vector<CmisLaneState>& states) {
  for (uint8_t lane = 0; lane < numHostLanes(); lane++) {
    if (!((1 << lane) & laneMask)) {
      continue;
    }
    auto dpState = getDatapathLaneStateLocked(lane, false);
    if (std::find(states.begin(), states.end(), dpState) == states.end()) {
      return false;
    }
  }
  return true;
}

void CmisModule::resetDataPath() {
  resetDataPathWithFunc();
}

void CmisModule::resetDataPathWithFunc(
    std::optional<std::function<void()>> afterDataPathDeinitFunc,
    uint8_t hostLaneMask) {
  if (flatMem_) {
    return;
  }

  uint8_t dataPathDeInitReg;
  readCmisField(CmisField::DATA_PATH_DEINIT, &dataPathDeInitReg);
  // First deactivate all the lanes
  uint8_t dataPathDeInit = dataPathDeInitReg | hostLaneMask;
  writeCmisField(CmisField::DATA_PATH_DEINIT, &dataPathDeInit);

  // Wait for all datapath state machines to get Deactivated
  const auto maxRetriesDeInit = maxRetriesWith500msDelay(/*init=*/false);

  auto retries = 0;
  while (retries++ < maxRetriesDeInit) {
    /* sleep override */
    usleep(kUsecDatapathStatePollTime);
    if (isDatapathUpdated(hostLaneMask, {CmisLaneState::DEACTIVATED})) {
      break;
    }
  }
  if (retries >= maxRetriesDeInit) {
    QSFP_LOG(ERR, this) << fmt::format(
        "Datapath could not deactivate even after waiting {:d} uSec",
        kUsecDatapathStateUpdateTime);
  }

  // Call the afterDataPathDeinitFunc() after detactivate all lanes
  if (afterDataPathDeinitFunc) {
    (*afterDataPathDeinitFunc)();
  }

  // Release the lanes from DeInit.
  dataPathDeInit = dataPathDeInitReg & ~(hostLaneMask);
  writeCmisField(CmisField::DATA_PATH_DEINIT, &dataPathDeInit);

  // Wait for the datapath to come out of deactivated state
  const auto maxRetriesInit = maxRetriesWith500msDelay(/*init=*/true);
  retries = 0;
  while (retries++ < maxRetriesInit) {
    /* sleep override */
    usleep(kUsecDatapathStatePollTime);
    if (isDatapathUpdated(
            hostLaneMask,
            {CmisLaneState::ACTIVATED, CmisLaneState::DATAPATH_INITIALIZED})) {
      break;
    }
  }
  if (retries >= maxRetriesInit) {
    QSFP_LOG(ERR, this) << fmt::format(
        "Datapath didn't come out of deactivated state even after waiting {:d} uSec",
        kUsecDatapathStateUpdateTime);
  }

  // Update the last datapath reset time for all the lanes in hostLaneMask
  for (int lane = 0; lane < CmisModule::kMaxOsfpNumLanes; lane++) {
    if ((1 << lane) & hostLaneMask) {
      lastDatapathResetTimes_[lane] = std::time(nullptr);
    }
  }
  QSFP_LOG(INFO, this) << folly::sformat(
      "DATA_PATH_DEINIT set and reset done for host lane mask 0x{:#x}",
      hostLaneMask);
}

/*
 * getDatapathLaneStateLocked
 *
 * Reads the datapath state for a given lane of transceiver either from HW or
 * from SW cache (default)
 */
CmisLaneState CmisModule::getDatapathLaneStateLocked(
    uint8_t lane,
    bool readFromCache) {
  CHECK_LE(lane, 8);
  if (!readFromCache) {
    uint8_t dataPathStates[4];
    readCmisField(CmisField::DATA_PATH_STATE, dataPathStates);
    auto laneDatapathState = dataPathStates[lane / 2];
    laneDatapathState = ((lane % 2) == 0) ? (laneDatapathState & 0xF)
                                          : ((laneDatapathState >> 4) & 0xF);
    return (CmisLaneState)laneDatapathState;
  } else {
    const uint8_t* data;
    int offset;
    int length;
    int dataAddress;
    getQsfpFieldAddress(
        CmisField::DATA_PATH_STATE, dataAddress, offset, length);
    data = getQsfpValuePtr(dataAddress, offset, length);
    auto laneDatapathState = data[lane / 2];
    laneDatapathState = ((lane % 2) == 0) ? (laneDatapathState & 0xF)
                                          : ((laneDatapathState >> 4) & 0xF);
    return (CmisLaneState)laneDatapathState;
  }
}

void CmisModule::updateVdmCacheLocked() {
  if (!isVdmSupported()) {
    QSFP_LOG(DBG5, this) << "Doesn't support VDM, skip updating VDM cache";
    return;
  }
  readCmisField(CmisField::PAGE_UPPER20H, page20_);
  readCmisField(CmisField::PAGE_UPPER21H, page21_);
  readCmisField(CmisField::PAGE_UPPER24H, page24_);
  readCmisField(CmisField::PAGE_UPPER25H, page25_);
  if (isVdmSupported(3)) {
    // Cache VDM group 3 page only if it is supported
    if (!staticPagesCached_) {
      readCmisField(CmisField::PAGE_UPPER22H, page22_);
      staticPagesCached_ = true;
    }
    readCmisField(CmisField::PAGE_UPPER26H, page26_);
  }
}

void CmisModule::updateCmisStateChanged(
    ModuleStatus& moduleStatus,
    std::optional<ModuleStatus> curModuleStatus) {
  if (!present_) {
    return;
  }
  // If `moduleStatus` already has true `cmisStateChanged`, no need to update
  if (auto cmisStateChanged = moduleStatus.cmisStateChanged();
      cmisStateChanged && *cmisStateChanged) {
    return;
  }
  // Otherwise, update it using curModuleStatus
  if (curModuleStatus) {
    if (auto curCmisStateChanged = curModuleStatus->cmisStateChanged()) {
      moduleStatus.cmisStateChanged() = *curCmisStateChanged;
    }
  } else {
    // If curModuleStatus is nullopt, we call getModuleStateChanged() to get
    // the latest moduleStateChanged
    moduleStatus.cmisStateChanged() = getModuleStateChanged();
  }
}

bool CmisModule::supportRemediate() {
  return supportRemediate_;
}

/*
 * setTransceiverTx
 *
 * Set the Tx output enabled/disabled for the given channels of a transceiver
 * in either line side or host side. For line side, this will cause LOS on the
 * peer optics Rx. For host side, this will cause LOS on the corresponding
 * IPHY lanes or the XPHY lanes in case of system with external PHY
 */
bool CmisModule::setTransceiverTxLocked(
    const std::string& portName,
    phy::Side side,
    std::optional<uint8_t> userChannelMask,
    bool enable) {
  // Get the list of lanes to disable/enable the Tx output
  auto tcvrLanes = getTcvrLanesForPort(portName, side);
  if (tcvrLanes.empty()) {
    QSFP_LOG(ERR, this) << fmt::format(
        "Empty lane list for port {:s}", portName);
    return false;
  }

  return setTransceiverTxImplLocked(tcvrLanes, side, userChannelMask, enable);
}

bool CmisModule::setTransceiverTxImplLocked(
    const std::set<uint8_t>& tcvrLanes,
    phy::Side side,
    std::optional<uint8_t> userChannelMask,
    bool enable) {
  if (tcvrLanes.empty()) {
    QSFP_LOG(ERR, this) << "Empty lane list";
    return false;
  }

  // Check if the module supports Tx control feature first
  if (!isTransceiverFeatureSupported(TransceiverFeature::TX_DISABLE, side)) {
    throw FbossError(
        fmt::format(
            "Module {:s} does not support transceiver TX output control on {:s}",
            qsfpImpl_->getName(),
            ((side == phy::Side::LINE) ? "Line" : "System")));
  }

  // Set the Tx output register for these lanes in given direction
  auto txDisableRegister =
      (side == phy::Side::LINE) ? CmisField::TX_DISABLE : CmisField::RX_DISABLE;
  uint8_t txDisableVal;

  readCmisField(txDisableRegister, &txDisableVal);

  txDisableVal =
      setTxChannelMask(tcvrLanes, userChannelMask, enable, txDisableVal);

  writeCmisField(txDisableRegister, &txDisableVal);
  return true;
}

bool CmisModule::upgradeFirmwareLockedImpl(FbossFirmware* fbossFw) const {
  QSFP_LOG(INFO, this) << "Upgrading CMIS Module Firmware";

  auto fwUpgradeObj =
      std::make_unique<CmisFirmwareUpgrader>(qsfpImpl_, getID(), fbossFw);

  bool ret = fwUpgradeObj->cmisModuleFirmwareUpgrade();
  return ret;
}

/*
 * setTransceiverLoopbackLocked
 *
 * Sets or resets the loopback on the given lanes for the SW Port on system
 * or line side of the Transceiver. The System side loopback set should bring
 * up the NPU port. The Line side loopback set should bring up the peer port.
 */
void CmisModule::setTransceiverLoopbackLocked(
    const std::string& portName,
    phy::Side side,
    bool setLoopback) {
  // Get the list of lanes to disable/enable the loopback
  auto tcvrLanes = getTcvrLanesForPort(portName, side);
  if (tcvrLanes.empty()) {
    QSFP_LOG(ERR, this) << fmt::format(
        "No {:s} lanes available for port {:s}",
        (side == phy::Side::SYSTEM ? "HOST" : "LINE"),
        portName);
    return;
  }

  // Check if the module supports system or line side loopback
  if (!isTransceiverFeatureSupported(TransceiverFeature::LOOPBACK, side)) {
    throw FbossError(
        fmt::format(
            "Module {:s} does not support transceiver Loopback on {:s}",
            portName,
            ((side == phy::Side::LINE) ? "Line" : "System")));
  }

  auto regField = (side == phy::Side::SYSTEM) ? CmisField::MEDIA_FAR_LB_EN
                                              : CmisField::MEDIA_NEAR_LB_EN;
  uint8_t hostOrMediaInputLbEnable;
  readCmisField(regField, &hostOrMediaInputLbEnable);

  hostOrMediaInputLbEnable = setTxChannelMask(
      tcvrLanes, std::nullopt, !setLoopback, hostOrMediaInputLbEnable);

  writeCmisField(regField, &hostOrMediaInputLbEnable);
}

/*
 * f16ToDouble
 *
 * Convert CMIS VDM F16 type (16 bit) to a floating point number
 */
double CmisModule::f16ToDouble(uint8_t byte0, uint8_t byte1) {
  double ber;
  int expon = byte0 >> 3;
  expon -= 24;
  int mant = ((byte0 & 0x7) << 8) | byte1;
  ber = mant * exp10(expon);
  return ber;
}

/*
 * getVdmDataValPtr
 *
 * Returns the VDM data value pointer from register cache along with VDM data
 * length, otherwise returns null
 */
std::pair<std::optional<const uint8_t*>, int> CmisModule::getVdmDataValPtr(
    VdmConfigType vdmConf) {
  const uint8_t* data;
  int offset;
  int length;
  int dataAddress;

  if (!cacheIsValid()) {
    return std::make_pair(std::nullopt, 0);
  }
  auto vdmDiagsValLocation = getVdmDiagsValLocation(vdmConf);
  if (vdmDiagsValLocation.vdmConfImplementedByModule) {
    dataAddress = static_cast<int>(vdmDiagsValLocation.vdmValPage);
    offset = vdmDiagsValLocation.vdmValOffset;
    length = vdmDiagsValLocation.vdmValLength;
    data = getQsfpValuePtr(dataAddress, offset, length);
    return std::make_pair(data, length);
  }
  return std::make_pair(std::nullopt, 0);
}

/*
 * fillVdmPerfMonitorSnr
 *
 * Private function to fill in the VDM performance monitor stats for SNR
 */
bool CmisModule::fillVdmPerfMonitorSnr(VdmPerfMonitorStats& vdmStats) {
  if (!isVdmSupported() || !cacheIsValid()) {
    return false;
  }

  // Get the SW Ports and the Channels for each port
  auto& portNameToMediaLanes = getPortNameToMediaLanes();

  // Fill in channel SNR Media In
  std::map<int, double> channelSnrMap;
  auto [data, length] = getVdmDataValPtr(SNR_MEDIA_IN);
  if (data) {
    for (auto lanes = 0; lanes < length / 2; lanes++) {
      double snr;
      snr = data.value()[lanes * 2] +
          (data.value()[lanes * 2 + 1] / kU16TypeLsbDivisor);
      channelSnrMap[lanes] = snr;
    }
  }
  for (auto& [portName, mediaLanes] : portNameToMediaLanes) {
    for (auto& mediaLane : mediaLanes) {
      vdmStats.mediaPortVdmStats()[portName].laneSNR()[mediaLane] =
          channelSnrMap[mediaLane];
    }
  }
  return true;
}

/*
 * fillVdmPerfMonitorBer
 *
 * Private function to fill in the VDM performance monitor stats for BER (Bit
 * Error Rate) on both Media and Host side
 */
bool CmisModule::fillVdmPerfMonitorBer(VdmPerfMonitorStats& vdmStats) {
  if (!isVdmSupported() || !cacheIsValid()) {
    return false;
  }

  // Get the SW Ports and the Channels for each port
  // Get the SW Ports and the Channels for each port
  auto& portNameToMediaLanes = getPortNameToMediaLanes();
  auto& portNameToHostLanes = getPortNameToHostLanes();

  // Lambda to extract BER or Frame Error values for a given VDM config type
  // on a SW Port
  auto captureVdmBerFrameErrorValues =
      [&](VdmConfigType vdmConfType, int startLane) -> std::optional<double> {
    auto [data, length] = getVdmDataValPtr(vdmConfType);
    if (data) {
      return f16ToDouble(
          data.value()[startLane * kVdmDescriptorLength],
          data.value()[startLane * kVdmDescriptorLength + 1]);
    }
    return std::nullopt;
  };

  // Fill in Media side per port values
  for (auto& [portName, mediaLanes] : portNameToMediaLanes) {
    auto startLane = *mediaLanes.begin();

    // Fill in Media Pre FEC BER values
    if (auto berVal = captureVdmBerFrameErrorValues(
            PRE_FEC_BER_MEDIA_IN_MIN, startLane)) {
      vdmStats.mediaPortVdmStats()[portName].datapathBER()->min() =
          berVal.value();
    }
    if (auto berVal = captureVdmBerFrameErrorValues(
            PRE_FEC_BER_MEDIA_IN_MAX, startLane)) {
      vdmStats.mediaPortVdmStats()[portName].datapathBER()->max() =
          berVal.value();
    }
    if (auto berVal = captureVdmBerFrameErrorValues(
            PRE_FEC_BER_MEDIA_IN_AVG, startLane)) {
      vdmStats.mediaPortVdmStats()[portName].datapathBER()->avg() =
          berVal.value();
    }
    if (auto berVal = captureVdmBerFrameErrorValues(
            PRE_FEC_BER_MEDIA_IN_CUR, startLane)) {
      vdmStats.mediaPortVdmStats()[portName].datapathBER()->cur() =
          berVal.value();
    }
  }

  // Fill in Host side per port values
  for (auto& [portName, hostLanes] : portNameToHostLanes) {
    auto startLane = *hostLanes.begin();

    // Fill in Host Pre FEC BER values
    if (auto berVal =
            captureVdmBerFrameErrorValues(PRE_FEC_BER_HOST_IN_MIN, startLane)) {
      vdmStats.hostPortVdmStats()[portName].datapathBER()->min() =
          berVal.value();
    }
    if (auto berVal =
            captureVdmBerFrameErrorValues(PRE_FEC_BER_HOST_IN_MAX, startLane)) {
      vdmStats.hostPortVdmStats()[portName].datapathBER()->max() =
          berVal.value();
    }
    if (auto berVal =
            captureVdmBerFrameErrorValues(PRE_FEC_BER_HOST_IN_AVG, startLane)) {
      vdmStats.hostPortVdmStats()[portName].datapathBER()->avg() =
          berVal.value();
    }
    if (auto berVal =
            captureVdmBerFrameErrorValues(PRE_FEC_BER_HOST_IN_CUR, startLane)) {
      vdmStats.hostPortVdmStats()[portName].datapathBER()->cur() =
          berVal.value();
    }
  }
  return true;
}

/*
 * fillVdmPerfMonitorFecErr
 *
 * Private function to fill in the VDM performance monitor stats for FEC Error
 * Rate (Post FEC BER) on both Media and Host side
 */
bool CmisModule::fillVdmPerfMonitorFecErr(VdmPerfMonitorStats& vdmStats) {
  if (!isVdmSupported() || !cacheIsValid()) {
    return false;
  }

  // Get the SW Ports and the Channels for each port
  auto& portNameToMediaLanes = getPortNameToMediaLanes();
  auto& portNameToHostLanes = getPortNameToHostLanes();

  // Lambda to extract BER or Frame Error values for a given VDM config type
  // on a SW Port
  auto captureVdmBerFrameErrorValues =
      [&](VdmConfigType vdmConfType, int startLane) -> std::optional<double> {
    auto [data, length] = getVdmDataValPtr(vdmConfType);
    if (data) {
      return f16ToDouble(
          data.value()[startLane * kVdmDescriptorLength],
          data.value()[startLane * kVdmDescriptorLength + 1]);
    }
    return std::nullopt;
  };

  // Fill in Media side per port values
  for (auto& [portName, mediaLanes] : portNameToMediaLanes) {
    auto startLane = *mediaLanes.begin();

    // Fill in Media Post FEC Errored Frames values
    if (auto errFrames =
            captureVdmBerFrameErrorValues(ERR_FRAME_MEDIA_IN_MIN, startLane)) {
      vdmStats.mediaPortVdmStats()[portName].datapathErroredFrames()->min() =
          errFrames.value();
    }
    if (auto errFrames =
            captureVdmBerFrameErrorValues(ERR_FRAME_MEDIA_IN_MAX, startLane)) {
      vdmStats.mediaPortVdmStats()[portName].datapathErroredFrames()->max() =
          errFrames.value();
    }
    if (auto errFrames =
            captureVdmBerFrameErrorValues(ERR_FRAME_MEDIA_IN_AVG, startLane)) {
      vdmStats.mediaPortVdmStats()[portName].datapathErroredFrames()->avg() =
          errFrames.value();
    }
    if (auto errFrames =
            captureVdmBerFrameErrorValues(ERR_FRAME_MEDIA_IN_CUR, startLane)) {
      vdmStats.mediaPortVdmStats()[portName].datapathErroredFrames()->cur() =
          errFrames.value();
    }
  }

  // Fill in Host side per port values
  for (auto& [portName, hostLanes] : portNameToHostLanes) {
    auto startLane = *hostLanes.begin();

    // Fill in Host Post FEC Errored Frame values
    if (auto errFrames =
            captureVdmBerFrameErrorValues(ERR_FRAME_HOST_IN_MIN, startLane)) {
      vdmStats.hostPortVdmStats()[portName].datapathErroredFrames()->min() =
          errFrames.value();
    }
    if (auto errFrames =
            captureVdmBerFrameErrorValues(ERR_FRAME_HOST_IN_MAX, startLane)) {
      vdmStats.hostPortVdmStats()[portName].datapathErroredFrames()->max() =
          errFrames.value();
    }
    if (auto errFrames =
            captureVdmBerFrameErrorValues(ERR_FRAME_HOST_IN_AVG, startLane)) {
      vdmStats.hostPortVdmStats()[portName].datapathErroredFrames()->avg() =
          errFrames.value();
    }
    if (auto errFrames =
            captureVdmBerFrameErrorValues(ERR_FRAME_HOST_IN_CUR, startLane)) {
      vdmStats.hostPortVdmStats()[portName].datapathErroredFrames()->cur() =
          errFrames.value();
    }
  }
  return true;
}

/*
 * fillVdmPerfMonitorFecTail
 *
 * Private function to fill in the VDM performance monitor stats for FEC Tail
 * on both Media and Host side
 */
bool CmisModule::fillVdmPerfMonitorFecTail(VdmPerfMonitorStats& vdmStats) {
  if (!isVdmSupported() || !cacheIsValid()) {
    return false;
  }

  // Get the SW Ports and the Channels for each port
  auto& portNameToMediaLanes = getPortNameToMediaLanes();
  auto& portNameToHostLanes = getPortNameToHostLanes();

  // Lambda to extract FEC tail for a given VDM config type on a SW Port
  auto captureVdmFecTailValues = [&](VdmConfigType vdmConfType,
                                     int startLane) -> std::optional<double> {
    auto [data, length] = getVdmDataValPtr(vdmConfType);
    if (data) {
      return data.value()[startLane * kVdmDescriptorLength] +
          data.value()[startLane * kVdmDescriptorLength + 1];
    }
    return std::nullopt;
  };

  // Fill in Media side per port values
  for (auto& [portName, mediaLanes] : portNameToMediaLanes) {
    auto startLane = *mediaLanes.begin();

    // Fill in Media FEC tail values
    if (auto fecTailMax =
            captureVdmFecTailValues(FEC_TAIL_MEDIA_IN_MAX, startLane)) {
      vdmStats.mediaPortVdmStats()[portName].fecTailMax() = fecTailMax.value();
      // FIXME: We should check FEC type and set the max supported FEC tail.
      // FEC Type is currently not available and hence hardcoding to 15 for
      // now
      vdmStats.mediaPortVdmStats()[portName].maxSupportedFecTail() =
          kMaxFecTailRs544;
    }
    if (auto fecTailCurr =
            captureVdmFecTailValues(FEC_TAIL_MEDIA_IN_CURR, startLane)) {
      vdmStats.mediaPortVdmStats()[portName].fecTailCurr() =
          fecTailCurr.value();
    }
  }

  // Fill in Host side per port values
  for (auto& [portName, hostLanes] : portNameToHostLanes) {
    auto startLane = *hostLanes.begin();

    // Fill in Host FEC tail values
    if (auto fecTailMax =
            captureVdmFecTailValues(FEC_TAIL_HOST_IN_MAX, startLane)) {
      vdmStats.hostPortVdmStats()[portName].fecTailMax() = fecTailMax.value();
      // FIXME: We should check FEC type and set the max supported FEC tail.
      // FEC Type is currently not available and hence hardcoding to 15 for
      // now
      vdmStats.hostPortVdmStats()[portName].maxSupportedFecTail() =
          kMaxFecTailRs544;
    }
    if (auto fecTailCurr =
            captureVdmFecTailValues(FEC_TAIL_HOST_IN_CURR, startLane)) {
      vdmStats.hostPortVdmStats()[portName].fecTailCurr() = fecTailCurr.value();
    }
  }
  return true;
}

/*
 * fillVdmPerfMonitorLtp
 *
 * Private function to fill in the VDM performance monitor stats for LTP
 * (Level Transition Parameter) on Media side
 */
bool CmisModule::fillVdmPerfMonitorLtp(VdmPerfMonitorStats& vdmStats) {
  if (!isVdmSupported() || !cacheIsValid()) {
    return false;
  }

  // Get the SW Ports and the Channels for each port
  auto& portNameToMediaLanes = getPortNameToMediaLanes();

  // Fill in channel LTP Media In
  std::map<int, double> channelLtpMap;
  auto [data, length] = getVdmDataValPtr(PAM4_LTP_MEDIA_IN);
  if (data) {
    for (auto lanes = 0; lanes < length / 2; lanes++) {
      double ltp;
      ltp = data.value()[lanes * 2] +
          (data.value()[lanes * 2 + 1] / kU16TypeLsbDivisor);
      channelLtpMap[lanes] = ltp;
    }
  }
  for (auto& [portName, mediaLanes] : portNameToMediaLanes) {
    for (auto& mediaLane : mediaLanes) {
      vdmStats.mediaPortVdmStats()[portName].lanePam4LTP()[mediaLane] =
          channelLtpMap[mediaLane];
    }
  }
  return true;
}

/*
 * fillVdmPerfMonitorPam4Data
 *
 * Private function to fill in the VDM performance monitor stats for PAM4 like
 * each level standard deviation, MPI (Multi Path Interference) on Media side
 */
bool CmisModule::fillVdmPerfMonitorPam4Data(VdmPerfMonitorStats& vdmStats) {
  if (!isVdmSupported(3) || !cacheIsValid()) {
    return false;
  }

  // Get the SW Ports and the Channels for each port
  auto& portNameToMediaLanes = getPortNameToMediaLanes();

  // Fill in VDM Advance group3 performance monitoring info

  // Lambda to read the VDM PM value for the given VDM Config
  auto getVdmPmLaneValues =
      [&](VdmConfigType vdmConf) -> std::map<int, double> {
    std::map<int, double> pmMap;
    auto [data, length] = getVdmDataValPtr(vdmConf);
    if (data) {
      for (auto lanes = 0; lanes < length / 2; lanes++) {
        double pmVal;
        pmVal =
            f16ToDouble(data.value()[lanes * 2], data.value()[lanes * 2 + 1]);
        pmMap[lanes] = pmVal;
      }
    }
    return pmMap;
  };

  // PAM4 Level0, Level1, Level2 , Level3, MPI
  auto sdL0Map = getVdmPmLaneValues(PAM4_LEVEL0_STANDARD_DEVIATION_LINE);
  auto sdL1Map = getVdmPmLaneValues(PAM4_LEVEL1_STANDARD_DEVIATION_LINE);
  auto sdL2Map = getVdmPmLaneValues(PAM4_LEVEL2_STANDARD_DEVIATION_LINE);
  auto sdL3Map = getVdmPmLaneValues(PAM4_LEVEL3_STANDARD_DEVIATION_LINE);
  auto mpiMap = getVdmPmLaneValues(PAM4_MPI_LINE);
  for (auto& [portName, mediaLanes] : portNameToMediaLanes) {
    for (auto& mediaLane : mediaLanes) {
      vdmStats.mediaPortVdmStats()[portName].lanePam4Level0SD()[mediaLane] =
          sdL0Map[mediaLane];
      vdmStats.mediaPortVdmStats()[portName].lanePam4Level1SD()[mediaLane] =
          sdL1Map[mediaLane];
      vdmStats.mediaPortVdmStats()[portName].lanePam4Level2SD()[mediaLane] =
          sdL2Map[mediaLane];
      vdmStats.mediaPortVdmStats()[portName].lanePam4Level3SD()[mediaLane] =
          sdL3Map[mediaLane];
      vdmStats.mediaPortVdmStats()[portName].lanePam4MPI()[mediaLane] =
          mpiMap[mediaLane];
    }
  }
  return true;
}

/*
 * fillVdmPerfMonitorPam4AlarmData
 *
 * Reads and processes the latched alarm and warning flags for PAM4 MPI
 * values.
 *
 * These flags are stored in VDM page 0x2C, bytes 208-211, with the following
 * bit layout:
 *
 * Byte 208:
 *   - bit 0: High alarm for lane 0
 *   - bit 2: High warning for lane 0
 *   - bit 4: High alarm for lane 1
 *   - bit 6: High warning for lane 1
 *
 * Byte 209:
 *   - bit 0: High alarm for lane 2
 *   - bit 2: High warning for lane 2
 *   - bit 4: High alarm for lane 3
 *   - bit 6: High warning for lane 3
 *
 * Byte 210:
 *   - bit 0: High alarm for lane 4
 *   - bit 2: High warning for lane 4
 *   - bit 4: High alarm for lane 5
 *   - bit 6: High warning for lane 5
 *
 * Byte 211:
 *   - bit 0: High alarm for lane 6
 *   - bit 2: High warning for lane 6
 *   - bit 4: High alarm for lane 7
 *   - bit 6: High warning for lane 7
 *
 * The function reads these flags and stores them in the VdmPerfMonitorStats
 * structure.
 */
bool CmisModule::fillVdmPerfMonitorPam4AlarmData(
    VdmPerfMonitorStats& vdmStats) {
  if (!isVdmSupported(3) || !cacheIsValid()) {
    return false;
  }
  auto vdmConfStatus = getVdmDiagsValLocation(PAM4_MPI_LINE);
  if (!vdmConfStatus.vdmConfImplementedByModule) {
    return false;
  }
  // For Line-side histogram based MPI_metric value, the ThresholdSetID ID
  // should be 34
  uint8_t thresholdSetID = vdmConfStatus.localThresholdSetID + 33;
  if (thresholdSetID != 34) {
    QSFP_LOG(WARN, this)
        << "Skipping reading PAM4_MPI_LINE warning/alarms, unexpected ThresholdSetID: "
        << static_cast<int>(thresholdSetID);
    return false;
  }

  auto& portNameToMediaLanes = getPortNameToMediaLanes();
  // VDM page 0x2C byte 208-211 contain the Alarms and Warnings flags for MPI
  std::array<uint8_t, 4>
      alarmData; // Buffer to read alarm data (4 bytes for 8 lanes)
  try {
    // Read the alarm data using the CmisField we defined
    readCmisField(CmisField::PAM4_MPI_ALARMS, alarmData.data());
  } catch (const std::exception& ex) {
    QSFP_LOG(ERR, this) << "Error reading MPI alarm data: " << ex.what();
    return false;
  }
  // Process the alarm/warning flags for each lane
  for (auto& [portName, mediaLanes] : portNameToMediaLanes) {
    for (auto& mediaLane : mediaLanes) {
      if (mediaLane > 7) {
        // We only support up to 8 lanes (0-7)
        continue;
      }
      // Calculate which byte and bit position to read for this lane
      int byteOffset = mediaLane / 2;
      int bitOffset = (mediaLane % 2) * 4; // 0 or 4 depending on even/odd lane

      // Extract alarm and warning flags directly
      bool alarmHigh = (alarmData[byteOffset] & (1 << bitOffset)) != 0;
      bool warnHigh = (alarmData[byteOffset] & (1 << (bitOffset + 2))) != 0;

      QSFP_LOG(DBG3, this) << "Lane " << mediaLane
                           << " MPI Alarm: " << (alarmHigh ? "true" : "false")
                           << " MPI Warning: " << (warnHigh ? "true" : "false");

      FlagLevels flags;
      flags.alarm()->high() = alarmHigh;
      flags.alarm()->low() = false; // Low alarm not used for MPI
      flags.warn()->high() = warnHigh;
      flags.warn()->low() = false; // Low warning not used for MPI

      vdmStats.mediaPortVdmStats()[portName].lanePam4MPIFlags()[mediaLane] =
          flags;
    }
  }
  return true;
}

} // namespace fboss
} // namespace facebook
