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

#include "fboss/agent/state/NeighborTable.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/FbossError.h"

#include <chrono>
#include <mutex>
#include <folly/IPAddress.h>

namespace facebook { namespace fboss {

/*
 * This class is used to manage expiration of neighbor entries (ARP and NDP).
 * It maintains a sorted list (timestamps_) that stores the last time a neighbor
 * entry was hit in hardware. We only expire an entry if it has not been hit in
 * hardware for at least 'timeout' seconds.
 *
 * There are two main operations: refreshing entries and expiring entries.
 * Refreshing updates our sorted list based on whether the entry was hit in
 * hardware. Expiring goes through the list and removes any entries that have
 * not been hit in 'timeout' seconds.
 *
 * It is expected that refreshing will occur with much higher frequency than
 * expiration, though that is up to the caller to implement.
 *
 * This class also listens for state updates to ensure that timestamps_ is kept
 * up to date.
 *
 * NOTE: It is expected that all of these calls are called in the update thread,
 * so we don't require locking around timestamps_ and tsmap_.
 */
template <typename NTABLE>
class NeighborExpirer {
 public:
  typedef NTABLE NTable;
  NeighborExpirer(SwSwitch* sw, std::chrono::seconds timeout);
  ~NeighborExpirer() {}

  void addNewEntries(const NodeMapDelta<NTable>& delta);
  void expireEntries(NTable* ntable);

  bool willExpireEntries();
  void refreshEntries();

 private:
  typedef std::chrono::time_point<std::chrono::steady_clock> TimePoint;
  typedef typename NTable::Entry::AddressType AddressType;
  typedef std::pair<TimePoint, AddressType> TimeIp;
  typedef typename std::list<TimeIp>::iterator TsIter;

  void refreshEntry(AddressType ip);
  void addEntry(AddressType ip);
  void removeEntryIf(AddressType ip);
  void removeEntry(AddressType ip);

  SwSwitch* sw_;
  std::chrono::seconds timeout_;
  std::list<TimeIp> timestamps_;
  std::unordered_map<AddressType, TsIter> tsmap_;
  std::mutex tsMutex_;
};

template <typename NTABLE>
NeighborExpirer<NTABLE>::NeighborExpirer(SwSwitch* sw,
                                         std::chrono::seconds timeout)
    : sw_(sw),
      timeout_(timeout) {
  VLOG(3) << "Started neighbor expirer with timeout: " << timeout_.count();
}

template <typename NTABLE>
void NeighborExpirer<NTABLE>::addEntry(AddressType ip) {
  DCHECK(tsmap_.find(ip) == tsmap_.end());

  auto now = std::chrono::steady_clock::now();
  timestamps_.emplace_back(now, ip);

  // update the map to include the iterator we just added
  auto tsIt = timestamps_.end();
  tsmap_.emplace(ip, std::move(--tsIt));
}

template <typename NTABLE>
void NeighborExpirer<NTABLE>::removeEntryIf(AddressType ip) {
  auto it = tsmap_.find(ip);
  if (it != tsmap_.end()) {
    timestamps_.erase(it->second);
    tsmap_.erase(it);
  }
}

template <typename NTABLE>
void NeighborExpirer<NTABLE>::removeEntry(AddressType ip) {
  auto it = tsmap_.find(ip);
  if (it != tsmap_.end()) {
    timestamps_.erase(it->second);
    tsmap_.erase(it);
  } else {
    throw FbossError("Tried to remove non existent entry from NeighborExpirer");
  }
}

/*
 * Updates our internal data structures in response to a state change.
 */
template <typename NTABLE>
void NeighborExpirer<NTABLE>::addNewEntries(
    const NodeMapDelta<NTable>& delta) {
  std::lock_guard<std::mutex> g(tsMutex_);
  for (const auto &entry : delta) {
    auto newEntry = entry.getNew();
    auto oldEntry = entry.getOld();

    if (!newEntry) {
      removeEntryIf(oldEntry->getIP());
      continue;
    }

    if (!oldEntry) {
      // new entry. Add timestamp to updater (if not pending)
      if (!newEntry->isPending()) {
        addEntry(newEntry->getIP());
      }
    } else {
      // Only add timestamp if we are converting a pending entry
      if (!newEntry->isPending() && oldEntry->isPending()) {
        addEntry(newEntry->getIP());
      }
    }
  }
}

/*
 * This moves the entry to the end of the list and updates the timestamp. It
 * also updates the tsmap_ entry to point to the new iterator.
 */
template <typename NTABLE>
void NeighborExpirer<NTABLE>::refreshEntry(AddressType ip) {
  auto it = tsmap_.find(ip);
  if (it == tsmap_.end()) {
    return;
  }

  timestamps_.erase(it->second);
  tsmap_.erase(it);
  addEntry(ip);
}

/*
 * Checks the hw hit bit of every entry and refreshes the last access time of an
 * entry if it was hit in hardware.
 */
template <typename NTABLE>
void NeighborExpirer<NTABLE>::refreshEntries() {
  std::lock_guard<std::mutex> g(tsMutex_);
  auto size = timestamps_.size();
  auto it = timestamps_.begin();
  for (int i = 0; i < size; ++i) {
    auto timeIp = *it;
    auto ip = timeIp.second;
    auto baseIp = folly::IPAddress(ip);
    ++it;

    // TODO: Assume vrf 0 for now
    if (sw_->neighborEntryHit(RouterID(0), baseIp)) {
      // The entry was hit. Refresh the entry.
      refreshEntry(ip);
    }
  }
}

/*
 * Expires all entries that have not been hit in hardware in the last 'timeout_'
 * seconds.
 */
template <typename NTABLE>
void NeighborExpirer<NTABLE>::expireEntries(
    NTable* ntable) {
  std::lock_guard<std::mutex> g(tsMutex_);
  auto now = std::chrono::steady_clock::now();
  for (auto it = timestamps_.begin(); it != timestamps_.end();) {
    auto timeIp = *it;
    auto time = timeIp.first;
    auto ip = timeIp.second;
    auto lifetime =
      std::chrono::duration_cast<std::chrono::seconds>(now - time);

    if (lifetime >= timeout_) {
      VLOG(3) << "Expiring neighbor entry with address " << ip.str();
      ++it;
      removeEntry(ip);
      ntable->removeEntry(ip);
    } else {
      // Since timeouts_ is sorted by time, we know we don't have to expire
      // any more entries
      break;
    }
  }
}

template <typename NTABLE>
bool NeighborExpirer<NTABLE>::willExpireEntries() {
  std::lock_guard<std::mutex> g(tsMutex_);
  if (timestamps_.empty()) {
    return false;
  }

  auto now = std::chrono::steady_clock::now();
  auto time = timestamps_.front().first;
  auto lifetime = std::chrono::duration_cast<std::chrono::seconds>(now - time);
  return lifetime >= timeout_;
}

}} // facebook::fboss
