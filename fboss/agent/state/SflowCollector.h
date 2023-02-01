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

#include <memory>
#include <string>
#include <utility>

#include <folly/SocketAddress.h>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

struct SflowCollectorFields
    : public ThriftyFields<SflowCollectorFields, state::SflowCollectorFields> {
  SflowCollectorFields(const std::string& ip, const uint16_t port) {
    auto address = folly::SocketAddress(ip, port);
    writableData().id() = folly::to<std::string>(
        address.getFullyQualified(), ':', address.getPort());
    state::SocketAddress socketAddr;
    *socketAddr.host() = address.getFullyQualified();
    *socketAddr.port() = port;
    writableData().address() = socketAddr;
  }

  template <typename Fn>
  void forEachChild(Fn) {}

  state::SflowCollectorFields toThrift() const override {
    return data();
  }
  static SflowCollectorFields fromThrift(
      state::SflowCollectorFields const& sflowCollectorThrift);
  static folly::dynamic migrateToThrifty(folly::dynamic const& dyn);
  static void migrateFromThrifty(folly::dynamic& dyn);
  folly::dynamic toFollyDynamicLegacy() const;
  static SflowCollectorFields fromFollyDynamicLegacy(
      const folly::dynamic& sflowCollectorJson);

  bool operator==(const SflowCollectorFields& other) const {
    return data() == other.data();
  }
};

USE_THRIFT_COW(SflowCollector)

/*
 * SflowCollector stores the IP and port of a UDP-based collector of sFlow
 * samples.
 */
class SflowCollector
    : public ThriftStructNode<SflowCollector, state::SflowCollectorFields> {
 public:
  using Base = ThriftStructNode<SflowCollector, state::SflowCollectorFields>;
  using LegacyFields = SflowCollectorFields;
  SflowCollector(std::string ip, uint16_t port);

  const std::string& getID() const {
    return cref<switch_state_tags::id>()->cref();
  }

  const folly::SocketAddress getAddress() const {
    const auto& host =
        cref<switch_state_tags::address>()->cref<switch_state_tags::host>();
    const auto& port =
        cref<switch_state_tags::address>()->cref<switch_state_tags::port>();
    return folly::SocketAddress(host->cref(), port->cref());
  }

  static std::shared_ptr<SflowCollector> fromFollyDynamic(
      const folly::dynamic& dyn) {
    auto fields = LegacyFields::fromFollyDynamic(dyn);
    auto obj = fields.toThrift();
    return std::make_shared<SflowCollector>(std::move(obj));
  }

  static std::shared_ptr<SflowCollector> fromFollyDynamicLegacy(
      const folly::dynamic& dyn) {
    auto fields = LegacyFields::fromFollyDynamicLegacy(dyn);
    auto obj = fields.toThrift();
    return std::make_shared<SflowCollector>(std::move(obj));
  }

  folly::dynamic toFollyDynamic() const override {
    auto fields = LegacyFields::fromThrift(this->toThrift());
    return fields.toFollyDynamic();
  }

  folly::dynamic toFollyDynamicLegacy() const {
    auto fields = LegacyFields::fromThrift(this->toThrift());
    return fields.toFollyDynamicLegacy();
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
