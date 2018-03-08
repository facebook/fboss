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

class LinkAggregationManager : public AutoRegisterStateObserver {
 public:
  explicit LinkAggregationManager(SwSwitch* sw);
  ~LinkAggregationManager() override;

  void stateUpdated(const StateDelta& delta) override;
  void handlePacket(std::unique_ptr<RxPacket> pkt, folly::io::Cursor c);

  void populatePartnerPair(PortID portID, LacpPartnerPair& partnerPair);
  void populatePartnerPairs(std::vector<LacpPartnerPair>& partnerPairs);

  // TODO(samank): output data structure generic
  template <typename Iterator>
  std::vector<std::shared_ptr<LacpController>> getControllersFor(
      folly::Range<Iterator> ports) {
    // Although this method is thread-safe, it is only invoked from a Selector
    // object, which should always be executing over the LACP EVB
    CHECK(sw_->getLacpEvb()->inRunningEventBaseThread());

    std::vector<std::shared_ptr<LacpController>> controllers(
        std::distance(ports.begin(), ports.end()));

    folly::SharedMutexWritePriority::ReadHolder g(&controllersLock_);

    // TODO(samank): Rerwite as an O(N + M) algorithm
    for (auto i = 0; i < controllers.size(); ++i) {
      auto it = portToController_.find(ports[i]);
      CHECK(it != portToController_.end());

      controllers[i] = it->second;
    }

    // TODO(samank): does this move?
    return controllers;
  }

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
  bool initialized_{false};
  SwSwitch* sw_{nullptr};
};
} // namespace fboss
} // namespace facebook
