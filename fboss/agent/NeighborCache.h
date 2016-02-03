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

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/NeighborCacheImpl-defs.h"
#include "fboss/agent/NeighborCacheEntry.h"

#include <chrono>
#include <folly/Memory.h>
#include <folly/MacAddress.h>
#include <folly/IPAddress.h>
#include <folly/io/async/EventBase.h>

namespace facebook { namespace fboss {

/*
 * This class manages the sw state of the neighbor tables. All of the
 * actual work is done by NeighborCacheImpl. This class is used to
 * provide the locking for those calls. All calls into the cache should
 * go through this class and SHOULD NOT call into the NeighborCacheImpl
 * directly. NeighborCacheImpl assumes that it does not need to worry
 * about synchronization because of this.
 *
 * This class wraps the common logic for a NeighborCache. It is meant to be
 * extended for ARP/NDP specific caches.
 */
template <typename NTable>
class NeighborCache {
  friend class NeighborCacheEntry<NTable>;
 public:
  typedef typename NTable::Entry::AddressType AddressType;

  virtual ~NeighborCache() {}

  bool flushEntryBlocking (AddressType ip) {
    std::lock_guard<std::mutex> g(cacheLock_);
    return impl_->flushEntryBlocking(ip);
  }
  void repopulate(std::shared_ptr<NTable> table) {
    std::lock_guard<std::mutex> g(cacheLock_);
    impl_->repopulate(table);
  }
  void setIntfID(const InterfaceID intfID) {
    impl_->setIntfID(intfID);
  }

 protected:
  // protected constructor since this is only meant to be inherited from
  NeighborCache(SwSwitch* sw,
                VlanID vlanID,
                InterfaceID intfID,
                std::chrono::seconds timeout,
                uint32_t maxNeighborProbes,
                std::chrono::seconds staleEntryInterval)
      : sw_(sw),
        timeout_(timeout),
        maxNeighborProbes_(maxNeighborProbes),
        staleEntryInterval_(staleEntryInterval),
        impl_(folly::make_unique<NeighborCacheImpl<NTable>>(this, sw,
                                                            vlanID, intfID)) {}

  // Methods useful for subclasses
  void setPendingEntry(AddressType ip, bool program = true) {
    std::lock_guard<std::mutex> g(cacheLock_);
    impl_->setPendingEntry(ip, program);
  }

  void setExistingEntry(AddressType ip,
                        folly::MacAddress mac,
                        PortID port,
                        NeighborEntryState state) {
    std::lock_guard<std::mutex> g(cacheLock_);
    impl_->setExistingEntry(ip, mac, port, state);
  }

  void setEntry(AddressType ip,
                folly::MacAddress mac,
                PortID port,
                NeighborEntryState state) {
    std::lock_guard<std::mutex> g(cacheLock_);
    impl_->setEntry(ip, mac, port, state);
  }

  bool isSolicited(AddressType ip) {
    std::lock_guard<std::mutex> g(cacheLock_);
    return impl_->isSolicited(ip);
  }

  SwSwitch* getSw() const {
    return sw_;
  }

  InterfaceID getIntfID() const {
    return impl_->getIntfID();
  }

  VlanID getVlanID() const {
    return impl_->getVlanID();
  }

  std::chrono::seconds getBaseTimeout() const {
    return timeout_;
  }

  std::chrono::seconds getStaleEntryInterval() const {
    return staleEntryInterval_;
  }

  uint32_t getMaxNeighborProbes() const {
    return maxNeighborProbes_;
  }

 private:
  // This should only be called by a NeighborCacheEntry
  virtual void probeFor(AddressType ip) const = 0;

  void flushEntry(AddressType ip) {
    std::lock_guard<std::mutex> g(cacheLock_);
    return impl_->flushEntry(ip);
  }

  void processEntry(AddressType ip) {
    std::lock_guard<std::mutex> g(cacheLock_);
    return impl_->processEntry(ip);
  }

  // Has the entry corresponding to ip has been hit in hw
  bool isHit(AddressType ip) {
    return sw_->getAndClearNeighborHit(RouterID(0), ip);
  }

  // Forbidden copy constructor and assignment operator
  NeighborCache(NeighborCache const &) = delete;
  NeighborCache& operator=(NeighborCache const &) = delete;

  SwSwitch* sw_;
  const std::chrono::seconds timeout_;
  const uint32_t maxNeighborProbes_{0};
  const std::chrono::seconds staleEntryInterval_;
  std::unique_ptr<NeighborCacheImpl<NTable>> impl_;
  std::mutex cacheLock_;
};

}} // facebook::fboss
