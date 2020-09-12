// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/platforms/wedge/utils/BcmLedUtils.h"

namespace facebook::fboss {

void BcmLedUtils::initLED0(int /*unit*/, folly::ByteRange /*code*/) {}
void BcmLedUtils::initLED1(int /*unit*/, folly::ByteRange /*code*/) {}
void BcmLedUtils::setGalaxyPortStatus(
    int /*unit*/,
    int /*port*/,
    uint32_t /*status*/) {}
uint32_t BcmLedUtils::getGalaxyPortStatus(int /*unit*/, int /*port*/) {
  return 0;
}
void BcmLedUtils::setWedge100PortStatus(
    int /*unit*/,
    int /*port*/,
    uint32_t /*status*/) {}
uint32_t BcmLedUtils::getWedge100PortStatus(int /*unit*/, int /*port*/) {
  return 0;
}
void BcmLedUtils::setWedge40PortStatus(
    int /*unit*/,
    int /*port*/,
    uint32_t /*status*/) {}
uint32_t BcmLedUtils::getWedge40PortStatus(int /*unit*/, int /*port*/) {
  return 0;
}
} // namespace facebook::fboss
