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

#include "fboss/agent/NeighborCacheImpl.h"
#include "fboss/agent/types.h"
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NeighborEntry.h"
#include <folly/MacAddress.h>
#include <folly/IPAddress.h>
#include <folly/io/async/EventBase.h>
#include <folly/futures/Future.h>

namespace facebook { namespace fboss {

template <typename NTable>
void NeighborCacheImpl<NTable>::programEntry(Entry* entry) {}


template <typename NTable>
void NeighborCacheImpl<NTable>::programPendingEntry(Entry* entry) {}

template <typename NTable>
NeighborCacheImpl<NTable>::~NeighborCacheImpl() {
  /* All the NeighborCacheEntries need to be destroyed on
   * the background thread. Because we do not want to exit
   * the destructor until all of the entries have stopped
   * executing, we use folly futures to wait for all entries
   * to destroy themselves.
   */
  std::vector<folly::Future<folly::Unit>> stopTasks;

  for (auto item : entries_) {
    auto addr = item.first;
    auto entry = item.second;

    std::function<void()> stopEntry = [this, entry]() {
      Entry::destroy(std::move(entry), sw_->getBackgroundEVB());
    };

    // Run the stop function in the background thread to
    // ensure it can be safely run
    auto f = via(sw_->getBackgroundEVB())
      .then(stopEntry)
      .onError([=](const std::exception& e) {
          LOG(FATAL) << "failed to stop NeighborCacheEntry w/ addr " << addr;
        });
    stopTasks.push_back(std::move(f));
  }

  // Ensure that all of the updaters have been stopped before we return
  folly::collectAll(stopTasks).get();
}

template <typename NTable>
void NeighborCacheImpl<NTable>::repopulate(std::shared_ptr<NTable> table) {
  for (auto it = table->begin(); it != table->end(); ++it) {
    auto entry = *it;
    auto state = entry->isPending() ? NeighborEntryState::INCOMPLETE :
      NeighborEntryState::STALE;
    setEntryInternal(*entry->getFields(), state, true, false);
  }
}

template <typename NTable>
void NeighborCacheImpl<NTable>::setEntry(AddressType ip,
                                         folly::MacAddress mac,
                                         PortID portID,
                                         NeighborEntryState state) {
  setEntryInternal(EntryFields(ip, mac, portID, intfID_), state);
}

template <typename NTable>
void NeighborCacheImpl<NTable>::setExistingEntry(AddressType ip,
                                                 folly::MacAddress mac,
                                                 PortID portID,
                                                 NeighborEntryState state) {
  setEntryInternal(EntryFields(ip, mac, portID, intfID_), state, false);
}

template <typename NTable>
void NeighborCacheImpl<NTable>::setEntryInternal(const EntryFields& fields,
                                             NeighborEntryState state,
                                             bool add,
                                             bool program) {
  auto entry = getCacheEntry(fields.ip);
  if (entry) {
    if (entry->fieldsMatch(fields)) {
      // Entry is up to date so no hw programming is needed.
      // However, we should update it's state
      entry->updateState(state);
      return;
    }
    entry->updateFields(fields);
    entry->updateState(state);
  } else if (add) {
    auto evb = sw_->getBackgroundEVB();
    setCacheEntry(
      std::make_shared<Entry>(fields, evb, cache_, state));
  } else {
    // no entry should exist
    return;
  }

  if (program) {
    programEntry(getCacheEntry(fields.ip));
  }
}

template <typename NTable>
void NeighborCacheImpl<NTable>::setPendingEntry(AddressType ip,
                                                bool program) {
  auto entry = getCacheEntry(ip);
  if (entry) {
    // Never overwrite an existing entry with a pending entry
    return;
  }

  // Add entry to cache
  auto evb = sw_->getBackgroundEVB();
  setCacheEntry(
    std::make_shared<Entry>(ip, intfID_, PENDING, evb, cache_));
  if (program) {
    programPendingEntry(getCacheEntry(ip));
  }
}

template <typename NTable>
void NeighborCacheImpl<NTable>::processEntry(AddressType ip) {
  auto entry = getCacheEntry(ip);
  if (entry) {
    entry->process();
    if (entry->getState() == NeighborEntryState::EXPIRED) {
      flushEntry(ip);
    }
  }
}

template <typename NTable>
NeighborCacheEntry<NTable>* NeighborCacheImpl<NTable>::getCacheEntry(
    AddressType ip) const {
  auto it = entries_.find(ip);
  if (it != entries_.end()) {
    return it->second.get();
  }
  return nullptr;
}

template <typename NTable>
void NeighborCacheImpl<NTable>::setCacheEntry(std::shared_ptr<Entry> entry) {
  entries_[entry->getIP()] = std::move(entry);
}

template <typename NTable>
bool NeighborCacheImpl<NTable>::removeEntry(AddressType ip) {
  auto it = entries_.find(ip);
  if (it == entries_.end()) {
    return false;
  }

  // This asynchronously destroys the entries. This is needed because
  // entries need to be destroyed on the background thread, but we
  // likely have the cache level lock here and the background thread could be
  // waiting for the lock. To avoid this deadlock scenario, we keep the entry
  // around in a shared_ptr for a bit longer and then destroy it later.
  Entry::destroy(std::move(it->second), sw_->getBackgroundEVB());

  entries_.erase(it);

  return true;
}

template <typename NTable>
bool NeighborCacheImpl<NTable>::flushEntryFromSwitchState(
    std::shared_ptr<SwitchState>* state,
    Vlan* vlan,
    AddressType ip) {
  // The entry should be removed from the cache before it is
  // flushed from the SwitchState
  DCHECK(!getCacheEntry(ip));

  auto* table = vlan->getNeighborTable<NTable>().get();
  const auto& entry = table->getNodeIf(ip);
  if (!entry) {
    return false;
  }

  table = table->modify(&vlan, state);
  table->removeNode(ip);
  return true;
}

template <typename NTable>
bool NeighborCacheImpl<NTable>::flushEntryBlocking(AddressType ip) {
  return flushEntry(ip, true);
}

template <typename NTable>
bool NeighborCacheImpl<NTable>::flushEntry(AddressType ip,
                                           bool blocking) {
  // remove from cache
  if (!removeEntry(ip)) {
    return false;
  }

  // flush from SwitchState
  bool flushed{false};
  auto updateFn =
    [this, ip, &flushed](const std::shared_ptr<SwitchState>& state)
        -> std::shared_ptr<SwitchState> {
      std::shared_ptr<SwitchState> newState{state};
      auto* vlan = state->getVlans()->getVlan(vlanID_).get();
      if (flushEntryFromSwitchState(&newState, vlan, ip)) {
        flushed = true;
        return newState;
      }
      return nullptr;
  };
  if (blocking) {
    sw_->updateStateBlocking("flush neighbor entry", std::move(updateFn));
    return flushed;
  }

  sw_->updateState("remove neighbor entry", std::move(updateFn));
  return true;
}


template <typename NTable>
bool NeighborCacheImpl<NTable>::isSolicited(AddressType ip) {
  // For now we assume that all entries that are either INCOMPLETE or PROBE
  // were solicited. We are sending out a request for these states every second
  // and are actively waiting for a reply so this is a reasonable assumption.
  auto entry = getCacheEntry(ip);
  return entry && entry->isProbing();
}

}} // facebook::fboss
