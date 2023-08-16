// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/module/QsfpHelper.h"

namespace facebook {
namespace fboss {

/*
 * setTxChannelMask
 *
 * Sets the channel mask for the Tx or Rx disable register data in the module.
 * If the user has provided the channel mask then it is set/reset otherwise all
 * the channels for a given software port in the transceiver is set/reset as per
 * argument
 */
uint8_t setTxChannelMask(
    const std::set<uint8_t>& tcvrLanes,
    std::optional<uint8_t> userChannelMask,
    bool enable,
    uint8_t txRxDisableData) {
  if (tcvrLanes.empty()) {
    // No lanes provided so return original data
    return txRxDisableData;
  }

  // Some sanity check
  if (userChannelMask.has_value()) {
    auto tempChannelMask = userChannelMask.value();
    uint8_t channelId = 0;
    while (tempChannelMask) {
      if (tempChannelMask & 0x1) {
        if (tcvrLanes.find(channelId) == tcvrLanes.end()) {
          // Sanity check fails so return original data
          return txRxDisableData;
        }
      }
      tempChannelMask >>= 1;
      channelId++;
    }
  }

  uint8_t appliedChannelMask;
  if (userChannelMask.has_value()) {
    appliedChannelMask = userChannelMask.value();
  } else {
    uint8_t channelMask = 0;
    for (auto lane : tcvrLanes) {
      channelMask |= (1 << lane);
    }
    appliedChannelMask = channelMask;
  }

  if (enable) {
    txRxDisableData &= ~appliedChannelMask;
  } else {
    txRxDisableData |= appliedChannelMask;
  }

  return txRxDisableData;
}

} // namespace fboss
} // namespace facebook
