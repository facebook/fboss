/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <folly/IPAddress.h>
#include <folly/json.h>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/rib/RouteTypes.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::rib {

template <typename AddrT>
class RibRoute {
 public:
  using Prefix = RoutePrefix<AddrT>;

  explicit RibRoute(const Prefix& prefix) : fields_(prefix) {}
  RibRoute(const Prefix& prefix, ClientID clientId, RouteNextHopEntry entry)
      : fields_(prefix, clientId, std::move(entry)) {}

  static RibRoute<AddrT> fromFollyDynamic(const folly::dynamic& json);

  folly::dynamic toFollyDynamic() const {
    return fields_.toFollyDynamic();
  }

  static RibRoute<AddrT> fromJson(const folly::fbstring& jsonStr) {
    return fromFollyDynamic(folly::parseJson(jsonStr));
  }

  RouteDetails toRouteDetails() const {
    return fields_.toRouteDetails();
  }

  const Prefix& prefix() const {
    return fields_.prefix;
  }
  bool isResolved() const {
    return fields_.isResolved();
  }
  bool isUnresolvable() const {
    return fields_.isUnresolvable();
  }
  bool isConnected() const {
    return fields_.isConnected();
  }
  bool isDrop() const {
    return fields_.isDrop();
  }
  bool isToCPU() const {
    return fields_.isToCPU();
  }
  bool isProcessing() const {
    return fields_.isProcessing();
  }
  bool needResolve() const {
    // not resolved, nor unresolvable, nor in processing
    return fields_.needResolve();
  }
  std::string str() const {
    return fields_.str();
  }
  // Return the forwarding info for this route
  const RouteNextHopEntry& getForwardInfo() const {
    return fields_.fwd;
  }
  const RouteNextHopEntry* FOLLY_NULLABLE
  getEntryForClient(ClientID clientId) const {
    return fields_.getEntryForClient(clientId);
  }
  std::pair<ClientID, const RouteNextHopEntry*> getBestEntry() const {
    return fields_.getBestEntry();
  }
  bool hasNoEntry() const {
    return fields_.hasNoEntry();
  }

  bool has(ClientID clientId, const RouteNextHopEntry& entry) const {
    return fields_.has(clientId, entry);
  }

  bool isSame(const RibRoute* rt) const;

  void setProcessing() {
    fields_.setProcessing();
  }
  void setConnected() {
    fields_.setConnected();
  }
  void setResolved(RouteNextHopEntry fwd) {
    fields_.setResolved(std::move(fwd));
  }
  void setUnresolvable() {
    fields_.setUnresolvable();
  }
  void clearForward() {
    fields_.clearForward();
  }

  void update(ClientID clientId, RouteNextHopEntry entry) {
    fields_.update(clientId, std::move(entry));
  }

  void delEntryForClient(ClientID clientId) {
    fields_.delEntryForClient(clientId);
  }

  std::optional<cfg::AclLookupClass> getClassID() const {
    return fields_.getClassID();
  }
  void setClassID(std::optional<cfg::AclLookupClass> classID) {
    fields_.setClassID(std::move(classID));
  }

  bool operator==(const RibRoute& rf) const;

 private:
  facebook::fboss::RouteFields<AddrT> fields_;
};

using RibRouteV4 = RibRoute<folly::IPAddressV4>;
using RibRouteV6 = RibRoute<folly::IPAddressV6>;

} // namespace facebook::fboss::rib
