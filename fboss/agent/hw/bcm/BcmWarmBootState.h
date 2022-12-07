// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/dynamic.h>

namespace facebook::fboss {

/*
 * a class that represents a state of BcmSwitch, which can not be constructed
 * from hardware or SDK APIs, either because required information is unavailable
 * in hardware or SDK.
 */
class BcmSwitchIf;
class BcmLabeledTunnel;

class BcmWarmBootState {
 public:
  explicit BcmWarmBootState(const BcmSwitchIf* hw) : hw_(hw) {}
  folly::dynamic hostTableToFollyDynamic() const;

  template <typename EgressT>
  folly::dynamic egressToFollyDynamic(const EgressT* egress) const;

  folly::dynamic mplsNextHopsToFollyDynamic() const;

  folly::dynamic intfTableToFollyDynamic() const;

  folly::dynamic qosTableToFollyDynamic() const;

  folly::dynamic teFlowToFollyDynamic() const;

  folly::dynamic udfToFollyDynamic() const;

 private:
  template <typename Key, typename Value>
  folly::dynamic toFollyDynamic(
      const Key& key,
      const std::shared_ptr<Value>& value) const;

  folly::dynamic mplsTunnelToFollyDynamic(BcmLabeledTunnel* tunnel) const;

  const BcmSwitchIf* hw_;
};

} // namespace facebook::fboss
