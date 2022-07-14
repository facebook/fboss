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

#include <string>
#include <utility>

#include <folly/SocketAddress.h>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

struct SflowCollectorFields : public BetterThriftyFields<
                                  SflowCollectorFields,
                                  state::SflowCollectorFields> {
  SflowCollectorFields(const std::string& ip, const uint16_t port) {
    auto address = folly::SocketAddress(ip, port);
    *data.id() = folly::to<std::string>(
        address.getFullyQualified(), ':', address.getPort());
    state::SocketAddress socketAddr;
    *socketAddr.host() = address.getFullyQualified();
    *socketAddr.port() = port;
    *data.address() = socketAddr;
  }

  template <typename Fn>
  void forEachChild(Fn) {}

  static SflowCollectorFields fromThrift(
      state::SflowCollectorFields const& sflowCollectorThrift);
  static folly::dynamic migrateToThrifty(folly::dynamic const& dyn);
  static void migrateFromThrifty(folly::dynamic& dyn);
  folly::dynamic toFollyDynamicLegacy() const;
  static SflowCollectorFields fromFollyDynamicLegacy(
      const folly::dynamic& sflowCollectorJson);
};

/*
 * SflowCollector stores the IP and port of a UDP-based collector of sFlow
 * samples.
 */
class SflowCollector : public ThriftyBaseT<
                           state::SflowCollectorFields,
                           SflowCollector,
                           SflowCollectorFields> {
 public:
  SflowCollector(const std::string& ip, const uint16_t port);

  const std::string& getID() const {
    return *getFields()->data.id();
  }

  const folly::SocketAddress getAddress() const {
    return folly::SocketAddress(
        *getFields()->data.address()->host(),
        *getFields()->data.address()->port());
  }

 private:
  // Inherit the constructors required for clone()
  using ThriftyBaseT<
      state::SflowCollectorFields,
      SflowCollector,
      SflowCollectorFields>::ThriftyBaseT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
