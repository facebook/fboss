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

#include "fboss/agent/types.h"
#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/StateObserver.h"

#include <boost/container/flat_map.hpp>

#include <folly/SharedMutex.h>
#include <folly/io/Cursor.h>

#include <memory>
#include <vector>

namespace facebook {
namespace fboss {

class LacpController;
class LacpPartnerPair;
class RxPacket;
class StateDelta;
class SwSwitch;

struct LacpServicerIf {
  LacpServicerIf() {}
  virtual ~LacpServicerIf() {}

  virtual bool transmit(LACPDU lacpdu, PortID portID) = 0;
  virtual void enableForwarding(PortID portID, AggregatePortID aggPortID) = 0;
  virtual void disableForwarding(PortID portID, AggregatePortID aggPortID) = 0;
  // If Selector was a static member of LinkAggregationManager, this wouldn't be
  // necessary
  virtual std::vector<std::shared_ptr<LacpController>> getControllersFor(
      folly::Range<std::vector<PortID>::const_iterator> ports) = 0;
};

class LinkAggregationManager : public AutoRegisterStateObserver,
                               public LacpServicerIf {
 public:
  explicit LinkAggregationManager(SwSwitch* sw);
  ~LinkAggregationManager() override;

  void stateUpdated(const StateDelta& delta) override;
  void handlePacket(std::unique_ptr<RxPacket> pkt, folly::io::Cursor c);

  void populatePartnerPair(PortID portID, LacpPartnerPair& partnerPair);
  void populatePartnerPairs(std::vector<LacpPartnerPair>& partnerPairs);

  bool transmit(LACPDU lacpdu, PortID portID) override;
  void enableForwarding(PortID portID, AggregatePortID aggPortID) override;
  void disableForwarding(PortID portID, AggregatePortID aggPortID) override;
  std::vector<std::shared_ptr<LacpController>> getControllersFor(
      folly::Range<std::vector<PortID>::const_iterator> ports) override;

 private:
  void aggregatePortRemoved(const std::shared_ptr<AggregatePort>& aggPort);
  void aggregatePortAdded(const std::shared_ptr<AggregatePort>& aggPort);
  void aggregatePortChanged(
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort);
  void portChanged(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  // Forbidden copy constructor and assignment operator
  LinkAggregationManager(LinkAggregationManager const&) = delete;
  LinkAggregationManager& operator=(LinkAggregationManager const&) = delete;

  using PortIDToController =
      boost::container::flat_map<PortID, std::shared_ptr<LacpController>>;
  friend std::ostream& operator<<(
      std::ostream& out,
      const PortIDToController::iterator& it);

  PortIDToController portToController_;
  mutable folly::SharedMutexWritePriority controllersLock_;
  SwSwitch* sw_{nullptr};
};
} // namespace fboss
} // namespace facebook
