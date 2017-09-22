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
#include <opennsl/types.h>
}

#include <boost/container/flat_map.hpp>
#include <folly/dynamic.h>

#include "fboss/agent/types.h"

namespace facebook {
namespace fboss {

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

  opennsl_trunk_t getBcmTrunkId(AggregatePortID id) const {
    return static_cast<opennsl_trunk_t>(id);
  }
  AggregatePortID getAggregatePortId(opennsl_trunk_t trunk) const {
    return AggregatePortID(trunk);
  }

  // TODO(samank): Fill in method
  // Serialize to folly::dynamic
  folly::dynamic toFollyDynamic() const;

  opennsl_trunk_t linkDownHwNotLocked(opennsl_port_t port);

 private:
  // Forbidden copy constructor and assignment operator
  BcmTrunkTable(const BcmTrunkTable&) = delete;
  BcmTrunkTable& operator=(const BcmTrunkTable&) = delete;

  boost::container::flat_map<AggregatePortID, std::unique_ptr<BcmTrunk>>
      trunks_;
  const BcmSwitch* const hw_{nullptr};

  // Setup trunking machinery
  void setupTrunking();

  // State that stores if the BCM trunk has been initialized.
  bool isBcmHWTrunkInitialized_ = false;
};
}
} // namespace facebook::fboss
