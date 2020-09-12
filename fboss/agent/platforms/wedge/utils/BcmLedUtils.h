// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Range.h>

namespace facebook::fboss {

class BcmLedUtils {
 public:
  static void initLED0(int unit, folly::ByteRange code);
  static void initLED1(int unit, folly::ByteRange code);
  // galaxy
  static void setGalaxyPortStatus(int unit, int port, uint32_t status);
  static uint32_t getGalaxyPortStatus(int unit, int port);

  // wedge100
  static void setWedge100PortStatus(int unit, int port, uint32_t status);
  static uint32_t getWedge100PortStatus(int unit, int port);

  // wedge40
  static void setWedge40PortStatus(int unit, int port, uint32_t status);
  static uint32_t getWedge40PortStatus(int unit, int port);
};

} // namespace facebook::fboss
