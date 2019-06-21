// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/dynamic.h>

namespace facebook {
namespace fboss {
/*
 * a class that represents a state of BcmSwitch, which can not be constructed
 * from hardware or SDK APIs, either because required information is unavailable
 * in hardware or SDK.
 */
class BcmSwitchIf;

class BcmWarmBootState {
 public:
  explicit BcmWarmBootState(const BcmSwitchIf* hw) : hw_(hw) {}
  folly::dynamic hostTableToFollyDynamic() const;

 private:
  const BcmSwitchIf* hw_;
};

} // namespace fboss
} // namespace facebook
