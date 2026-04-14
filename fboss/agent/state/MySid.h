// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <common/network/if/gen-cpp2/Address_types.h>
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
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

  std::optional<NextHopSetID> getResolvedNextHopsId() const {
    if (auto id = safe_cref<switch_state_tags::resolvedNextHopsId>()) {
      return NextHopSetID(id->cref());
    }
    return std::nullopt;
  }

  void setResolvedNextHopsId(std::optional<NextHopSetID> id) {
    if (id) {
      set<switch_state_tags::resolvedNextHopsId>(static_cast<int64_t>(*id));
    } else {
      ref<switch_state_tags::resolvedNextHopsId>().reset();
    }
  }

  std::optional<NextHopSetID> getUnresolveNextHopsId() const {
    if (auto id = safe_cref<switch_state_tags::unresolveNextHopsId>()) {
      return NextHopSetID(id->cref());
    }
    return std::nullopt;
  }

  void setUnresolveNextHopsId(std::optional<NextHopSetID> id) {
    if (id) {
      set<switch_state_tags::unresolveNextHopsId>(static_cast<int64_t>(*id));
    } else {
      ref<switch_state_tags::unresolveNextHopsId>().reset();
    }
  }

  bool resolved() const {
    return getType() == MySidType::DECAPSULATE_AND_LOOKUP ||
        getResolvedNextHopsId().has_value();
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
