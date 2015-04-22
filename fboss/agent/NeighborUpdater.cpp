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
#include <folly/futures/Future.h>

using std::chrono::seconds;
using std::shared_ptr;
using boost::container::flat_map;
using folly::Future;
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

  shared_ptr<SwitchState>
  prunePendingEntries(const shared_ptr<SwitchState>& state);

  bool pendingEntriesExist() const;

  virtual void timeoutExpired() noexcept {
    if (pendingEntriesExist()) {
      sw_->updateState("Remove pending Arp entries", prunePendingEntries_);
    }
    scheduleTimeout(interval_);
  }

  VlanID const vlan_;
  SwSwitch* const sw_{nullptr};
  StateUpdateFn prunePendingEntries_;
  seconds interval_;
};

NeighborUpdaterImpl::NeighborUpdaterImpl(VlanID vlan, SwSwitch *sw,
                                         const SwitchState* state)
  : AsyncTimeout(sw->getBackgroundEVB()),
    vlan_(vlan),
    sw_(sw),
    interval_(state->getArpAgerInterval()) {
  prunePendingEntries_ = std::bind(&NeighborUpdaterImpl::prunePendingEntries,
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

void NeighborUpdaterImpl::stateChanged(const StateDelta& delta) {
  // Does nothing now. We may want to subscribe to changes in the configured
  // interval/timeouts here if we decide  want to be able to change these
  // values on the fly
}

shared_ptr<SwitchState>
NeighborUpdaterImpl::prunePendingEntries(const shared_ptr<SwitchState>& state) {
  shared_ptr<SwitchState> newState{state};

  bool modified = false;
  auto vlanIf = state->getVlans()->getVlanIf(vlan_);
  if (vlanIf) {
    auto vlan = vlanIf.get();

    auto arpTable = vlan->getArpTable().get();
    if (arpTable->hasPendingEntries()) {
      auto newArpTable = arpTable->modify(&vlan, &newState);
      if (newArpTable->prunePendingEntries()) {
        modified = true;
      }
    }

    auto ndpTable = vlan->getNdpTable().get();
    if (ndpTable->hasPendingEntries()) {
      auto newNdpTable = ndpTable->modify(&vlan, &newState);
      if (newNdpTable->prunePendingEntries()) {
        modified = true;
      }
    }
  }
  return modified ? newState : nullptr;
}

bool NeighborUpdaterImpl::pendingEntriesExist() const {
  auto state = sw_->getState();
  auto vlan = state->getVlans()->getVlanIf(vlan_);
  auto arpTable = vlan->getArpTable();
  auto ndpTable = vlan->getNdpTable();
  return arpTable->hasPendingEntries() || ndpTable->hasPendingEntries();
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
  collectAll(stopTasks.begin(), stopTasks.end()).get();
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
    updater->stateChanged(delta);
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
