/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <memory>

extern "C" {
#include <bcm/types.h>
}

#include <boost/container/flat_map.hpp>
#include <folly/concurrency/ConcurrentHashMap.h>
#include <folly/dynamic.h>

#include "fboss/agent/hw/bcm/MinimumLinkCountMap.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class AggregatePort;
class BcmSwitch;
class BcmTrunk;

class BcmTrunkTable {
 public:
  explicit BcmTrunkTable(const BcmSwitch* hw);
  virtual ~BcmTrunkTable();

  // Prior to the invocation of the following 3 functions, the global HW update
  // lock (BcmSwitch::lock_) should have been acquired.
  void addTrunk(const std::shared_ptr<AggregatePort>& aggPort);
  void programTrunk(
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort);
  void deleteTrunk(const std::shared_ptr<AggregatePort>& aggPort);

  bcm_trunk_t getBcmTrunkId(AggregatePortID id) const;
  AggregatePortID getAggregatePortId(bcm_trunk_t trunk) const;

  size_t numTrunkPorts() const {
    return trunks_.size();
  }

  // TODO(samank): Fill in method
  // Serialize to folly::dynamic
  folly::dynamic toFollyDynamic() const;

  bcm_trunk_t linkDownHwNotLocked(bcm_port_t port) const;

  void updateStats();

  bool isMinLinkMet(bcm_trunk_t trunk) const;

 private:
  // Forbidden copy constructor and assignment operator
  BcmTrunkTable(const BcmTrunkTable&) = delete;
  BcmTrunkTable& operator=(const BcmTrunkTable&) = delete;

  boost::container::flat_map<AggregatePortID, std::unique_ptr<BcmTrunk>>
      trunks_;
  const BcmSwitch* const hw_{nullptr};

  TrunkToMinimumLinkCountMap trunkToMinLinkCount_;

  // Used for thread-safe id translation
  folly::ConcurrentHashMap<AggregatePortID, bcm_trunk_t> tgidLookup_;

 public:
  using iterator = decltype(trunks_)::iterator;
  using const_iterator = decltype(trunks_)::const_iterator;
  iterator begin() {
    return trunks_.begin();
  }
  iterator end() {
    return trunks_.end();
  }
  const_iterator cbegin() const {
    return trunks_.cbegin();
  }
  const_iterator cend() const {
    return trunks_.cend();
  }
  const_iterator begin() const {
    return cbegin();
  }
  const_iterator end() const {
    return cend();
  }
};

} // namespace facebook::fboss
