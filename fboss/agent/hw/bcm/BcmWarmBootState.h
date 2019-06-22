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
class BcmHost;
class BcmSwitchIf;
class BcmHostKey;

class BcmWarmBootState {
 public:
  explicit BcmWarmBootState(const BcmSwitchIf* hw) : hw_(hw) {}
  folly::dynamic hostTableToFollyDynamic() const;

 private:
  folly::dynamic bcmHostToFollyDynamic(
      const BcmHostKey& hostKey,
      const std::shared_ptr<BcmHost>& host) const;
  const BcmSwitchIf* hw_;
};

} // namespace fboss
} // namespace facebook
