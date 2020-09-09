// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Range.h>

namespace facebook::fboss {

class BcmLedUtils {
 public:
  static void initLED0(int unit, folly::ByteRange code);
  static void initLED1(int unit, folly::ByteRange code);
};

} // namespace facebook::fboss
