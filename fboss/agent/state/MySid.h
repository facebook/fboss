// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <common/network/if/gen-cpp2/Address_types.h>
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

USE_THRIFT_COW(MySid);

class MySid : public ThriftStructNode<MySid, state::MySidFields> {
 public:
  using Base = ThriftStructNode<MySid, state::MySidFields>;

  std::string getID() const {
    auto prefix = safe_cref<switch_state_tags::mySid>();
    auto thriftPrefix = prefix->toThrift();
    auto ip = network::toIPAddress(*thriftPrefix.prefixAddress());
    auto len = *thriftPrefix.prefixLength();
    return folly::IPAddress::networkToString(
        std::make_pair(ip, static_cast<uint8_t>(len)));
  }

  MySidType getType() const {
    return get<switch_state_tags::type>()->cref();
  }

  void setType(MySidType type) {
    set<switch_state_tags::type>(type);
  }

  folly::CIDRNetwork getMySid() const {
    auto thriftPrefix = get<switch_state_tags::mySid>()->toThrift();
    auto ip = network::toIPAddress(*thriftPrefix.prefixAddress());
    auto len = *thriftPrefix.prefixLength();
    return std::make_pair(ip, static_cast<uint8_t>(len));
  }

  const RouteNextHopEntry* getResolvedNextHop() const {
    if (auto nhop = safe_cref<switch_state_tags::resolvedNextHop>()) {
      return &(*nhop);
    }
    return nullptr;
  }

  void setResolvedNextHop(const std::optional<RouteNextHopEntry>& nhop) {
    if (nhop) {
      set<switch_state_tags::resolvedNextHop>(nhop->toThrift());
    } else {
      ref<switch_state_tags::resolvedNextHop>().reset();
    }
  }

  const RouteNextHopEntry* getUnresolvedNextHop() const {
    if (auto nhop = safe_cref<switch_state_tags::unresolveNextHop>()) {
      return &(*nhop);
    }
    return nullptr;
  }

  void setUnresolvedNextHop(const std::optional<RouteNextHopEntry>& nhop) {
    if (nhop) {
      set<switch_state_tags::unresolveNextHop>(nhop->toThrift());
    } else {
      ref<switch_state_tags::unresolveNextHop>().reset();
    }
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
