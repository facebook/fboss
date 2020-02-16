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

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/types.h"
#include "fboss/lib/RefMap.h"

#include "folly/IPAddress.h"

#include <memory>

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class ResolvedNextHop;

using SaiIpNextHop = SaiObject<SaiIpNextHopTraits>;
using SaiMplsNextHop = SaiObject<SaiMplsNextHopTraits>;

/*
 * SaiNextHopHandle holds reference to either ip next hop or mpls next hop
 * This is done to simplify handling of multipath groups which may need to hold
 * both ip next hops or mpls next hops or both together. This is tightly wrapped
 * class, and clients of this class need not make any distinction between ip
 * next hop and mpls next hop. It only provides handle to underlying adapter
 * key.
 * The only operations this class should allow are creation, destruction and
 * fetching an underlying SAI object id. This means that one of the two next hop
 * SAI objects either exist in SDK/ASIC or they don't.
 */
class SaiNextHopHandle {
 public:
  explicit SaiNextHopHandle(const SaiIpNextHopTraits::AdapterHostKey& key);
  explicit SaiNextHopHandle(const SaiMplsNextHopTraits::AdapterHostKey& key);
  NextHopSaiId adapterKey() const {
    return id_;
  }
  SaiNextHopHandle(const SaiNextHopHandle&) = delete;
  SaiNextHopHandle& operator=(const SaiNextHopHandle&) = delete;
  SaiNextHopHandle(SaiNextHopHandle&&) = delete;
  SaiNextHopHandle& operator==(SaiNextHopHandle&&) = delete;

 private:
  // TODO(pshaikh): define object type for object with condition attribute and
  // its adapter host key
  using SaiNextHop = std::
      variant<std::shared_ptr<SaiIpNextHop>, std::shared_ptr<SaiMplsNextHop>>;
  SaiNextHop nexthop_;
  NextHopSaiId id_{0};
};

class SaiNextHopManager {
 public:
  SaiNextHopManager(SaiManagerTable* managerTable, const SaiPlatform* platform);
  std::shared_ptr<SaiIpNextHop> addNextHop(
      RouterInterfaceSaiId routerInterfaceId,
      const folly::IPAddress& ip);

  /* return next hop handle here, based on resolved next hop. resulting next hop
   * handle will be either ip next hop or mpls next hop depending on properties
   * of resolved next hop */
  std::shared_ptr<SaiNextHopHandle> refOrEmplace(
      const ResolvedNextHop& swNextHop);

 private:
  /* emplace ip next hop */
  std::shared_ptr<SaiNextHopHandle> refOrEmplace(
      SaiRouterInterfaceTraits::AdapterKey interface,
      const folly::IPAddress& ip);
  /* emplace mpls next hop with single label */
  std::shared_ptr<SaiNextHopHandle> refOrEmplace(
      SaiRouterInterfaceTraits::AdapterKey interface,
      const folly::IPAddress& ip,
      LabelForwardingAction::Label label);
  /* emplace mpls next hop with label stack */
  std::shared_ptr<SaiNextHopHandle> refOrEmplace(
      SaiRouterInterfaceTraits::AdapterKey interface,
      const folly::IPAddress& ip,
      const LabelForwardingAction::LabelStack& stack);

  SaiManagerTable* managerTable_;
  const SaiPlatform* platform_;
  UnorderedRefMap<SaiIpNextHopTraits::AdapterHostKey, SaiNextHopHandle>
      ipNextHops_;
  UnorderedRefMap<SaiMplsNextHopTraits::AdapterHostKey, SaiNextHopHandle>
      mplsNextHops_;
};

} // namespace facebook::fboss
