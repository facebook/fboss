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

#include <boost/container/flat_map.hpp>
#include <list>
#include <mutex>
#include <string>
#include <utility>
#include "fboss/agent/ArpCache.h"
#include "fboss/agent/NdpCache.h"
#include "fboss/agent/NeighborUpdaterImpl.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class SwitchState;
class StateDelta;

/**
 * This class handles all updates to neighbor entries. Whenever we perform an
 * action or receive a packet that should update the Arp or Ndp tables in the
 * SwitchState, it goes through this class first, which then schedules some
 * work on the neighbor thread using the implementation methods present in
 * `NeighborUpdaterImpl`.
 *
 * Most methods in this class return a `folly::Future<>` which the caller can
 * use to wait for completion of the underlying execution.
 *
 * This class implements the StateObserver API and listens for all vlan added
 * or deleted events. It ignores all changes it receives related to arp/ndp
 * tables, as all those changes should originate from the caches stored in this
 * class. Everything else is forwarded to the `Impl` class on the neighbor
 * thread.
 */
class NeighborUpdater : public AutoRegisterStateObserver {
 private:
  using NeighborCaches = NeighborUpdaterImpl::NeighborCaches;

  std::shared_ptr<NeighborUpdaterImpl> impl_;
  SwSwitch* sw_{nullptr};

 public:
  explicit NeighborUpdater(SwSwitch* sw);
  ~NeighborUpdater() override;

  void waitForPendingUpdates();

  void stateUpdated(const StateDelta& delta) override;

  // Zero-cost forwarders. See comment in NeighborUpdater.def.
#define ARG_TEMPLATE_PARAMETER(TYPE, NAME) typename T_##NAME
#define ARG_RVALUE_REF_TYPE(TYPE, NAME) T_##NAME&& NAME
#define ARG_FORWARDER(TYPE, NAME) std::forward<T_##NAME>(NAME)
#define ARG_NAME_ONLY(TYPE, NAME) NAME
#define NEIGHBOR_UPDATER_METHOD(VISIBILITY, NAME, RETURN_TYPE, ...)           \
  VISIBILITY:                                                                 \
  template <ARG_LIST(ARG_TEMPLATE_PARAMETER, ##__VA_ARGS__)>                  \
  folly::Future<folly::lift_unit_t<RETURN_TYPE>> NAME(                        \
      ARG_LIST(ARG_RVALUE_REF_TYPE, ##__VA_ARGS__)) {                         \
    return folly::via(sw_->getNeighborCacheEvb(), [=, impl = this->impl_]() { \
      return impl->NAME(ARG_LIST(ARG_NAME_ONLY, ##__VA_ARGS__));              \
    });                                                                       \
  }
#define NEIGHBOR_UPDATER_METHOD_NO_ARGS(VISIBILITY, NAME, RETURN_TYPE)     \
  VISIBILITY:                                                              \
  folly::Future<folly::lift_unit_t<RETURN_TYPE>> NAME() {                  \
    return folly::via(sw_->getNeighborCacheEvb(), [impl = this->impl_]() { \
      return impl->NAME();                                                 \
    });                                                                    \
  }
#include "fboss/agent/NeighborUpdater.def"
#undef NEIGHBOR_UPDATER_METHOD

 public:
  template <typename AddrT>
  void updateEntryClassID(
      VlanID vlan,
      AddrT ip,
      std::optional<cfg::AclLookupClass> classID = std::nullopt) {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      updateArpEntryClassID(vlan, ip, classID);
    } else {
      updateNdpEntryClassID(vlan, ip, classID);
    }
  }

 private:
  std::shared_ptr<NeighborCaches> createCaches(
      const SwitchState* state,
      const Vlan* vlan);
  void portChanged(
      const std::shared_ptr<Port>& oldPort,
      const std::shared_ptr<Port>& newPort);
  void aggregatePortChanged(
      const std::shared_ptr<AggregatePort>& oldAggPort,
      const std::shared_ptr<AggregatePort>& newAggPort);
  void sendNeighborUpdates(const VlanDelta& delta);

  // Forbidden copy constructor and assignment operator
  NeighborUpdater(NeighborUpdater const&) = delete;
  NeighborUpdater& operator=(NeighborUpdater const&) = delete;
};

} // namespace facebook::fboss
