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
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/VlanMap.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/NeighborExpirer.h"
#include <folly/futures/Future.h>

#include <chrono>
#include <boost/container/flat_map.hpp>

using std::chrono::seconds;
using std::chrono::time_point;
using std::chrono::steady_clock;
using std::shared_ptr;
using boost::container::flat_map;
using folly::Future;
using folly::whenAll;

namespace facebook { namespace fboss {

class NeighborUpdaterImpl : private folly::AsyncTimeout {
 public:
  NeighborUpdaterImpl(VlanID vlan, SwSwitch *sw, const SwitchState* state);

  SwSwitch* getSw() const {
    return sw_;
  }

  static void start(void* arg);
  static void stop(void* arg);

  void stateChanged(const VlanDelta& delta);

 private:
  typedef std::function<
    std::shared_ptr<SwitchState>(const std::shared_ptr<SwitchState>&)>
    StateUpdateFn;
  typedef NeighborExpirer<ArpTable> ArpExpirer;
  typedef NeighborExpirer<NdpTable> NdpExpirer;

  NeighborUpdaterImpl(NeighborUpdaterImpl const &) = delete;
  NeighborUpdaterImpl& operator=(NeighborUpdaterImpl const &) = delete;

  bool willUpdateTables();
  void refreshEntries();

  shared_ptr<SwitchState>
  updateTables(const shared_ptr<SwitchState>& state);

  virtual void timeoutExpired() noexcept {
    refreshEntries();
    if (willUpdateTables()) {
      sw_->updateState("Updating neighbor tables", updateTables_);
    }
    scheduleTimeout(interval_);
  }

  VlanID const vlan_;
  SwSwitch* const sw_{nullptr};
  StateUpdateFn updateTables_;
  seconds interval_;
  ArpExpirer arpExpirer_;
  NdpExpirer ndpExpirer_;
};

NeighborUpdaterImpl::NeighborUpdaterImpl(VlanID vlanID, SwSwitch* sw,
                                         const SwitchState* state)
  : AsyncTimeout(sw->getBackgroundEVB()),
    vlan_(vlanID),
    sw_(sw),
    interval_(state->getArpAgerInterval()),
    arpExpirer_(sw, state->getArpTimeout()),
    ndpExpirer_(sw, state->getNdpTimeout()) {
  updateTables_ = std::bind(&NeighborUpdaterImpl::updateTables,
                            this, std::placeholders::_1);
}

void NeighborUpdaterImpl::start(void* arg) {
  auto* ntu = static_cast<NeighborUpdaterImpl*>(arg);
  ntu->scheduleTimeout(ntu->interval_);
}

void NeighborUpdaterImpl::stop(void* arg) {
  auto* ntu = static_cast<NeighborUpdaterImpl*>(arg);
  delete ntu;
}

void NeighborUpdaterImpl::stateChanged(const VlanDelta& delta) {
  // We may want to subscribe to changes in the configured
  // interval/timeouts here if we decide we want to be able to change these
  // values on the fly
  arpExpirer_.addNewEntries(delta.getArpDelta());
  ndpExpirer_.addNewEntries(delta.getNdpDelta());
}

shared_ptr<SwitchState>
NeighborUpdaterImpl::updateTables(const shared_ptr<SwitchState>& state) {
  auto vlan = state->getVlans()->getVlanIf(vlan_).get();
  if (!vlan) {
    return nullptr;
  }

  shared_ptr<SwitchState> newState{state};
  auto arpTable = vlan->getArpTable().get();
  auto newArpTable = arpTable->modify(&vlan, &newState);
  newArpTable->prunePendingEntries();
  arpExpirer_.expireEntries(newArpTable);

  auto ndpTable = vlan->getNdpTable().get();
  auto newNdpTable = ndpTable->modify(&vlan, &newState);
  newNdpTable->prunePendingEntries();
  ndpExpirer_.expireEntries(newNdpTable);

  return newState;
}

bool NeighborUpdaterImpl::willUpdateTables() {
  auto state = sw_->getState();
  auto vlan = state->getVlans()->getVlanIf(vlan_);
  auto arpTable = vlan->getArpTable();
  auto ndpTable = vlan->getNdpTable();
  return arpTable->hasPendingEntries() || ndpTable->hasPendingEntries() ||
    arpExpirer_.willExpireEntries() || ndpExpirer_.willExpireEntries();
}

void NeighborUpdaterImpl::refreshEntries() {
  arpExpirer_.refreshEntries();
  ndpExpirer_.refreshEntries();
}

NeighborUpdater::NeighborUpdater(SwSwitch* sw)
    : AutoRegisterStateObserver(sw, "NeighborUpdater"),
      sw_(sw) {}

NeighborUpdater::~NeighborUpdater() {
  std::vector<Future<void>> stopTasks;

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
  whenAll(stopTasks.begin(), stopTasks.end()).get();
}

void NeighborUpdater::stateUpdated(const StateDelta& delta) {
  CHECK(sw_->getUpdateEVB()->inRunningEventBaseThread());
  for (const auto& entry : delta.getVlansDelta()) {
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
    updater->stateChanged(entry);
  }
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
