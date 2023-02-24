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

USE_THRIFT_COW(SflowCollector)

/*
 * SflowCollector stores the IP and port of a UDP-based collector of sFlow
 * samples.
 */
class SflowCollector
    : public ThriftStructNode<SflowCollector, state::SflowCollectorFields> {
 public:
  using Base = ThriftStructNode<SflowCollector, state::SflowCollectorFields>;
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

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
