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

#include "fboss/agent/LacpTypes.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>

#include <folly/SharedMutex.h>
#include <folly/io/Cursor.h>

#include <memory>
#include <vector>

namespace facebook::fboss {

class LacpController;
class LacpPartnerPair;
class RxPacket;
class StateDelta;
class SwSwitch;

struct LacpServicerIf {
  LacpServicerIf() {}
  virtual ~LacpServicerIf() {}

  virtual bool transmit(LACPDU lacpdu, PortID portID) = 0;
  virtual void enableForwardingAndSetPartnerState(
      PortID portID,
      AggregatePortID aggPortID,
      const ParticipantInfo& partnerState) = 0;
  virtual void disableForwardingAndSetPartnerState(
      PortID portID,
      AggregatePortID aggPortID,
      const ParticipantInfo& partnerState) = 0;
  // If Selector was a static member of LinkAggregationManager, this wouldn't be
  // necessary
  virtual std::vector<std::shared_ptr<LacpController>> getControllersFor(
      folly::Range<std::vector<PortID>::const_iterator> ports) = 0;
};

class ProgramForwardingAndPartnerState {
 public:
  ProgramForwardingAndPartnerState(
      PortID portID,
      AggregatePortID aggPortID,
      AggregatePort::Forwarding fwdState,
      AggregatePort::PartnerState partnerState);
  std::shared_ptr<SwitchState> operator()(
      const std::shared_ptr<SwitchState>& state);

 private:
  PortID portID_;
  AggregatePortID aggregatePortID_;
  AggregatePort::Forwarding forwardingState_;
  AggregatePort::PartnerState partnerState_;
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
  void enableForwardingAndSetPartnerState(
      PortID portID,
      AggregatePortID aggPortID,
      const ParticipantInfo& partnerState) override;
  void disableForwardingAndSetPartnerState(
      PortID portID,
      AggregatePortID aggPortID,
      const ParticipantInfo& partnerState) override;
  std::vector<std::shared_ptr<LacpController>> getControllersFor(
      folly::Range<std::vector<PortID>::const_iterator> ports) override;

  static void recordStatistics(
      SwSwitch* sw,
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort);

 private:
  void aggregatePortRemoved(const std::shared_ptr<AggregatePort>& aggPort);
  void aggregatePortAdded(const std::shared_ptr<AggregatePort>& aggPort);
  void aggregatePortChanged(
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort);
  void portChanged(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);

  void updateAggregatePortStats(
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort);

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
  bool initialStateSynced_{false};
};

} // namespace facebook::fboss
