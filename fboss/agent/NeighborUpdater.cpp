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
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/Interface.h"
#include <folly/futures/Future.h>

using std::chrono::seconds;
using std::shared_ptr;
using boost::container::flat_map;
using folly::Future;
using folly::Unit;
using folly::collectAll;

namespace facebook { namespace fboss {

class NeighborUpdaterImpl : private folly::AsyncTimeout {
 public:
  NeighborUpdaterImpl(VlanID vlan, SwSwitch *sw, const SwitchState* state);

  SwSwitch* getSw() const {
    return sw_;
  }

  static void start(void* arg);
  static void stop(void* arg);

  void stateChanged(const StateDelta& delta);

 private:
  typedef std::function<
    std::shared_ptr<SwitchState>(const std::shared_ptr<SwitchState>&)>
    StateUpdateFn;

  NeighborUpdaterImpl(NeighborUpdaterImpl const &) = delete;
  NeighborUpdaterImpl& operator=(NeighborUpdaterImpl const &) = delete;

  template<typename TABLE, typename FUNC>
  void sendNeighborRequestHelper(const TABLE& table,
      const shared_ptr<Interface>& intf, const shared_ptr<Vlan>& vlan,
      FUNC& sendNeighborReq) const;

  void sendNeighborRequestForPendingEntriesHit() const;

  void timeoutExpired() noexcept override {
    sendNeighborRequestForPendingEntriesHit();
    scheduleTimeout(interval_);
  }

  VlanID const vlan_;
  SwSwitch* const sw_{nullptr};
  seconds interval_;
};

NeighborUpdaterImpl::NeighborUpdaterImpl(VlanID vlan, SwSwitch *sw,
                                         const SwitchState* state)
  : AsyncTimeout(sw->getBackgroundEVB()),
    vlan_(vlan),
    sw_(sw),
    // Check for hit pending entries every 1 sec
    interval_(1) {
}

void NeighborUpdaterImpl::start(void* arg) {
  auto* ntu = static_cast<NeighborUpdaterImpl*>(arg);
  ntu->scheduleTimeout(ntu->interval_);
}

void NeighborUpdaterImpl::stop(void* arg) {
  auto* ntu = static_cast<NeighborUpdaterImpl*>(arg);
  delete ntu;
}

void NeighborUpdaterImpl::stateChanged(const StateDelta& delta) {
  // Does nothing now. We may want to subscribe to changes in the configured
  // interval/timeouts here if we decide  want to be able to change these
  // values on the fly
}

template<typename TABLE, typename FUNC>
void NeighborUpdaterImpl::sendNeighborRequestHelper(
    const TABLE& table, const shared_ptr<Interface>& intf,
    const shared_ptr<Vlan>& vlan, FUNC& sendNeighborReq) const {

  for (const auto& entry : table) {
    if (entry->isPending() &&
      sw_->getAndClearNeighborHit(intf->getRouterID(),
        folly::IPAddress(entry->getIP()))) {
      VLOG (4) << " Pending neighbor entry for " << entry->getIP()
        << " was hit, sending neighbor request ";
      sendNeighborReq(intf, vlan, entry.get(), sw_);
    }
  }
}

void NeighborUpdaterImpl::sendNeighborRequestForPendingEntriesHit() const {
  static auto sendArpReq = [=] (const shared_ptr<Interface>& intf,
      const shared_ptr<Vlan>& vlan, const ArpEntry* entry, SwSwitch* sw) {
      auto source = intf->getAddressToReach(entry->getIP())->first.asV4();
      sw->getArpHandler()->sendArpRequest(vlan, source, entry->getIP());
  };
  static auto sendNdpReq = [=] (const shared_ptr<Interface>& intf,
      const shared_ptr<Vlan>& vlan, const NdpEntry* entry, SwSwitch* sw) {
      sw->getIPv6Handler()->sendNeighborSolicitation(entry->getIP(), vlan);
  };
  auto state = sw_->getState();
  const auto&  vlanMap = state->getVlans();
  for (auto& intf: *state->getInterfaces()) {
    auto vlan = vlanMap->getVlan(intf->getVlanID());
    // all interfaces must have a valid vlan
    CHECK(vlan);
    // Send arp for pending ARP entries that were hit
    sendNeighborRequestHelper(*vlan->getArpTable(), intf, vlan, sendArpReq);
    // Send ndp for pending NDP entries that were hit
    sendNeighborRequestHelper(*vlan->getNdpTable(), intf, vlan, sendNdpReq);
  }
}

NeighborUpdater::NeighborUpdater(SwSwitch* sw)
    : AutoRegisterStateObserver(sw, "NeighborUpdater"),
      sw_(sw) {}

NeighborUpdater::~NeighborUpdater() {
  std::vector<Future<Unit>> stopTasks;

  for (auto entry : updaters_) {
    auto vlan = entry.first;
    auto updater = entry.second;

    std::function<void()> stopUpdater = [updater]() {
      NeighborUpdaterImpl::stop(updater);
    };

    // Run the stop function in the background thread to
    // ensure it can be safely run
    auto f = via(sw_->getBackgroundEVB())
      .then(stopUpdater)
      .onError([=](const std::exception& e) {
        LOG(FATAL) << "failed to stop neighbor updater w/ vlan " << vlan;
      });
    stopTasks.push_back(std::move(f));
  }

  // Ensure that all of the updaters have been stopped before we return
  collectAll(stopTasks).get();
}

void NeighborUpdater::stateUpdated(const StateDelta& delta) {
  CHECK(sw_->getUpdateEVB()->inRunningEventBaseThread());
  for (const auto& entry : delta.getVlansDelta()) {
    sendNeighborUpdates(entry);
    auto oldEntry = entry.getOld();
    auto newEntry = entry.getNew();

    if (!newEntry) {
      vlanDeleted(oldEntry.get());
      continue;
    }

    if (!oldEntry) {
      vlanAdded(delta.newState().get(), newEntry.get());
    }

    auto res = updaters_.find(newEntry->getID());
    auto updater = res->second;
    updater->stateChanged(delta);
  }
}

template<typename T>
void collectPresenceChange(const T& delta,
                          std::vector<std::string>* added,
                          std::vector<std::string>* deleted) {
  for (const auto& entry : delta) {
    auto oldEntry = entry.getOld();
    auto newEntry = entry.getNew();
    if (oldEntry && !newEntry) {
      deleted->push_back(folly::to<std::string>(oldEntry->getIP()));
    } else if (newEntry && !oldEntry) {
      added->push_back(folly::to<std::string>(newEntry->getIP()));
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
  LOG(INFO) << "Got neighbor update";
  sw_->invokeNeighborListener(added, deleted);
}

void NeighborUpdater::vlanAdded(const SwitchState* state, const Vlan* vlan) {
  CHECK(sw_->getUpdateEVB()->inRunningEventBaseThread());
  auto updater = new NeighborUpdaterImpl(vlan->getID(), sw_, state);
  updaters_.emplace(vlan->getID(), updater);
  bool ret = sw_->getBackgroundEVB()->runInEventBaseThread(
    NeighborUpdaterImpl::start, updater);
  if (!ret) {
    delete updater;

    throw FbossError("failed to start neighbor updater");
  }
}

void NeighborUpdater::vlanDeleted(const Vlan* vlan) {
  CHECK(sw_->getUpdateEVB()->inRunningEventBaseThread());
  auto updater = updaters_.find(vlan->getID());
  if (updater != updaters_.end()) {
    updaters_.erase(updater);
    bool ret = sw_->getBackgroundEVB()->runInEventBaseThread(
      NeighborUpdaterImpl::stop, (*updater).second);
    if (!ret) {
      LOG(ERROR) << "failed to stop neighbor updater";
    }
  } else {
    // TODO(aeckert): May want to fatal here when an updater doesn't exist for a
    // specific vlan. Need to make sure that updaters are correctly created for
    // the initial SwitchState to avoid false positives
  }
}

}} // facebook::fboss
