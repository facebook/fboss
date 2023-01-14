// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/QsfpUtil.h"
#include <folly/logging/xlog.h>
#include <sysexits.h>

namespace facebook::fboss {

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

} // namespace facebook::fboss
