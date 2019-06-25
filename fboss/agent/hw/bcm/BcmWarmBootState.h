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

  template <typename EgressT>
  folly::dynamic egressToFollyDynamic(const EgressT* egress) const;

 private:
  template <typename Key, typename Value>
  folly::dynamic toFollyDynamic(
      const Key& key,
      const std::shared_ptr<Value>& value) const;

  const BcmSwitchIf* hw_;
};

} // namespace fboss
} // namespace facebook
