// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/led_service/FsdbSwitchStateSubscriber.h"

namespace facebook::fboss {

std::vector<std::string> FsdbSwitchStateSubscriber::getSwitchStatePath() {
  return {};
}
std::vector<std::string> FsdbSwitchStateSubscriber::getTransceiverStatePath() {
  return {};
}

} // namespace facebook::fboss
