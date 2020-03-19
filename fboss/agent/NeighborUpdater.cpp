/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "NeighborUpdater.h"
#include "fboss/agent/ArpCache.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/NdpCache.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

#include <boost/container/flat_map.hpp>
#include <folly/logging/xlog.h>
#include <list>
#include <mutex>
#include <string>
#include <vector>

using boost::container::flat_map;
using folly::IPAddress;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using std::shared_ptr;

namespace facebook::fboss {

using facebook::fboss::DeltaFunctions::forEachChanged;

NeighborUpdater::NeighborUpdater(SwSwitch* sw)
    : AutoRegisterStateObserver(sw, "NeighborUpdater"),
      impl_(std::make_shared<NeighborUpdaterImpl>()),
      sw_(sw) {}

NeighborUpdater::~NeighborUpdater() {
  // we make sure to destroy the NeighborUpdaterImpl on the neighbor
  // cache thread to avoid racing between background entry processing
  // and destruction.
  sw_->getNeighborCacheEvb()->runImmediatelyOrRunInEventBaseThreadAndWait(
      [this]() mutable { return impl_.reset(); });
}

void NeighborUpdater::waitForPendingUpdates() {
  folly::via(sw_->getNeighborCacheEvb(), [impl = this->impl_]() {}).get();
}

auto NeighborUpdater::createCaches(const SwitchState* state, const Vlan* vlan)
    -> std::shared_ptr<NeighborCaches> {
  auto caches = std::make_shared<NeighborCaches>(
      sw_, state, vlan->getID(), vlan->getName(), vlan->getInterfaceID());

  // We need to populate the caches from the SwitchState when a vlan is added
  // After this, we no longer process Arp or Ndp deltas for this vlan.
  caches->arpCache->repopulate(vlan->getArpTable());
  caches->ndpCache->repopulate(vlan->getNdpTable());

  return caches;
}

void NeighborUpdater::stateUpdated(const StateDelta& delta) {
  CHECK(sw_->getUpdateEvb()->inRunningEventBaseThread());
  for (const auto& entry : delta.getVlansDelta()) {
    sendNeighborUpdates(entry);
    auto oldEntry = entry.getOld();
    auto newEntry = entry.getNew();

    if (!newEntry) {
      vlanDeleted(oldEntry->getID());
    } else if (!oldEntry) {
      vlanAdded(
          newEntry->getID(),
          createCaches(delta.newState().get(), newEntry.get()));
    } else {
      if (newEntry->getInterfaceID() != oldEntry->getInterfaceID() ||
          newEntry->getName() != oldEntry->getName()) {
        // For now we only care about changes to the interfaceID and VlanName
        vlanChanged(
            newEntry->getID(), newEntry->getInterfaceID(), newEntry->getName());
      }
    }
  }

  const auto& oldState = delta.oldState();
  const auto& newState = delta.newState();
  if (oldState->getArpTimeout() != newState->getArpTimeout() ||
      oldState->getNdpTimeout() != newState->getNdpTimeout() ||
      oldState->getMaxNeighborProbes() != newState->getMaxNeighborProbes() ||
      oldState->getStaleEntryInterval() != newState->getStaleEntryInterval()) {
    timeoutsChanged(
        newState->getArpTimeout(),
        newState->getNdpTimeout(),
        newState->getStaleEntryInterval(),
        newState->getMaxNeighborProbes());
  }

  forEachChanged(delta.getPortsDelta(), &NeighborUpdater::portChanged, this);
  forEachChanged(
      delta.getAggregatePortsDelta(),
      &NeighborUpdater::aggregatePortChanged,
      this);
}

template <typename T>
void collectPresenceChange(
    const T& delta,
    std::vector<std::string>* added,
    std::vector<std::string>* deleted) {
  for (const auto& entry : delta) {
    auto oldEntry = entry.getOld();
    auto newEntry = entry.getNew();
    if (oldEntry && !newEntry) {
      if (oldEntry->nonZeroPort()) {
        deleted->push_back(folly::to<std::string>(oldEntry->getIP()));
      }
    } else if (newEntry && !oldEntry) {
      if (newEntry->nonZeroPort()) {
        added->push_back(folly::to<std::string>(newEntry->getIP()));
      }
    } else {
      if (oldEntry->zeroPort() && newEntry->nonZeroPort()) {
        // Entry was resolved, add it
        added->push_back(folly::to<std::string>(newEntry->getIP()));
      } else if (oldEntry->nonZeroPort() && newEntry->zeroPort()) {
        // Entry became unresolved, prune it
        deleted->push_back(folly::to<std::string>(oldEntry->getIP()));
      }
    }
  }
}

void NeighborUpdater::sendNeighborUpdates(const VlanDelta& delta) {
  std::vector<std::string> added;
  std::vector<std::string> deleted;
  collectPresenceChange(delta.getArpDelta(), &added, &deleted);
  collectPresenceChange(delta.getNdpDelta(), &added, &deleted);
  if (!(added.empty() && deleted.empty())) {
    sw_->invokeNeighborListener(added, deleted);
  }
}

void NeighborUpdater::portChanged(
    const std::shared_ptr<Port>& oldPort,
    const std::shared_ptr<Port>& newPort) {
  if (oldPort->getOperState() == Port::OperState::UP &&
      newPort->getOperState() == Port::OperState::DOWN) {
    // Fire explicit callback for purging neighbor entries.
    CHECK_EQ(oldPort->getID(), newPort->getID());
    auto portId = newPort->getID();

    XLOG(INFO) << "Purging neighbor entry for physical port " << portId;
    portDown(PortDescriptor(portId));
  }
}

void NeighborUpdater::aggregatePortChanged(
    const std::shared_ptr<AggregatePort>& oldAggPort,
    const std::shared_ptr<AggregatePort>& newAggPort) {
  CHECK_EQ(oldAggPort->getID(), newAggPort->getID());

  auto oldSubportRange = oldAggPort->sortedSubports();

  bool transitionedToDown = oldAggPort->isUp() && !newAggPort->isUp();
  bool membershipChanged = !std::equal(
      oldSubportRange.begin(),
      oldSubportRange.end(),
      newAggPort->sortedSubports().begin());

  if (transitionedToDown || membershipChanged) {
    auto aggregatePortID = newAggPort->getID();
    XLOG(INFO) << "Purging neighbor entry for aggregate port "
               << aggregatePortID;
    portDown(PortDescriptor(aggregatePortID));
  }
}

} // namespace facebook::fboss
