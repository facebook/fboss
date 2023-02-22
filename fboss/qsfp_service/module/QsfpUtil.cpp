// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/QsfpUtil.h"
#include <folly/logging/xlog.h>
#include <sysexits.h>

namespace facebook::fboss {

namespace {
constexpr int kNumRetryGetModuleType = 5;
}

QsfpUtil::QsfpUtil(
    ReadViaServiceFn readFn,
    WriteViaServiceFn writeFn,
    folly::EventBase* evb)
    : readViaServiceFn_(readFn), writeViaServiceFn_(writeFn), evb_(evb) {}

QsfpUtil::QsfpUtil(ReadViaDirectIoFn readFn, WriteViaDirectIoFn writeFn)
    : readViaDirectIoFn_(readFn), writeViaDirectIoFn_(writeFn) {
  directAccess_ = true;
}

/*
 * Set proper mask to disable selected channels
 */
void QsfpUtil::setChannelStateBitmask(
    uint8_t& data,
    uint32_t channel,
    bool disable) {
  const uint8_t maxChannels =
      (moduleType_ == TransceiverManagementInterface::CMIS)
      ? QsfpUtil::maxCmisChannels
      : QsfpUtil::maxSffChannels;
  const uint8_t channelMask =
      (moduleType_ == TransceiverManagementInterface::CMIS) ? 0xFF : 0xF;

  if (channel == 0) {
    // Disable/enable all channels
    data = disable ? channelMask : 0x0;
  } else if (channel < 1 || channel > maxChannels) {
    XLOG(ERR) << "Trying to " << (disable ? "disable" : "enable")
              << " invalid channel " << channel;
    return;
  } else {
    // Disable/enable a particular channel in module
    if (disable) {
      data |= (1 << (channel - 1));
    } else {
      data &= ~(1 << (channel - 1));
    }
  }
}

/*
 * This function returns the transceiver management interfaces
 * by reading the register 0 indirectly from modules. If there is an error,
 * this function returns an empty interface map.
 */
std::map<int32_t, TransceiverManagementInterface>
QsfpUtil::getModuleTypeViaService(const std::vector<unsigned int>& ports) {
  std::map<int32_t, TransceiverManagementInterface> moduleTypes;
  const int offset = 0;
  const int length = 1;
  const int page = 0;
  std::vector<int32_t> idx = zeroBasedPortIds(ports);
  std::map<int32_t, ReadResponse> readResp;
  readViaServiceFn_(
      idx,
      {TransceiverI2CApi::ADDR_QSFP, offset, length, page},
      *evb_,
      readResp);

  if (readResp.empty()) {
    XLOG(ERR) << "Indirect read error in getting module type";
    return moduleTypes;
  }

  for (const auto& response : readResp) {
    const auto moduleId = *(response.second.data()->data());
    const TransceiverManagementInterface modType =
        QsfpModule::getTransceiverManagementInterface(
            moduleId, response.first + 1);

    moduleTypes[response.first] = modType;
  }

  return moduleTypes;
}

/*
 * This function returns the transceiver management interface
 * by reading the register 0 directly from module
 */
TransceiverManagementInterface QsfpUtil::getModuleType(unsigned int port) {
  uint8_t moduleId = static_cast<uint8_t>(TransceiverModuleIdentifier::UNKNOWN);

  // Get the module id to differentiate between CMIS (0x1e) and SFF
  for (auto retry = 0; retry < kNumRetryGetModuleType; retry++) {
    try {
      readViaDirectIoFn_(port, {TransceiverI2CApi::ADDR_QSFP, 0, 1}, &moduleId);
    } catch (const I2cError& ex) {
      fprintf(
          stderr, "QSFP %d: not present or read error, retrying...\n", port);
    }
  }

  return QsfpModule::getTransceiverManagementInterface(moduleId, port);
}

std::vector<int32_t> QsfpUtil::zeroBasedPortIds(
    const std::vector<unsigned int>& ports) {
  std::vector<int32_t> idx;
  for (auto port : ports) {
    // Direct I2C bus starts from 1 instead of 0, however qsfp_service
    // index starts from 0. So here we try to comply to match that
    // behavior.
    idx.push_back(port - 1);
  }
  return idx;
}

} // namespace facebook::fboss
