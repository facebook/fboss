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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/RxPacket.h"
#include "fboss/agent/types.h"

#include "fboss/agent/PortDescriptorTemplate.h"

#include <folly/Conv.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

class PortDescriptor
    : public PortDescriptorTemplate<PortID, AggregatePortID, SystemPortID> {
 public:
  using BaseT = PortDescriptorTemplate<PortID, AggregatePortID, SystemPortID>;
  using BaseT::PortType;
  explicit PortDescriptor(PortID p) : BaseT(p) {}
  explicit PortDescriptor(AggregatePortID p) : BaseT(p) {}
  explicit PortDescriptor(SystemPortID p) : BaseT(p) {}
  explicit PortDescriptor(const BaseT& b) : BaseT(b) {}

  static PortDescriptor fromRxPacket(const RxPacket& pkt) {
    if (pkt.isFromAggregatePort()) {
      return PortDescriptor(AggregatePortID(pkt.getSrcAggregatePort()));
    } else {
      return PortDescriptor(PortID(pkt.getSrcPort()));
    }
  }

  static PortDescriptor fromThrift(const cfg::PortDescriptor& portTh) {
    return PortDescriptor(BaseT::fromThrift(portTh));
  }

  static PortDescriptor fromFollyDynamic(const folly::dynamic& descJSON) {
    return PortDescriptor(BaseT::fromFollyDynamic(descJSON));
  }
  cfg::PortDescriptor toCfgPortDescriptor() const {
    cfg::PortDescriptor cfgPort{};

    return cfgPort;
  }
  static PortDescriptor fromCfgCfgPortDescriptor(cfg::PortDescriptor cfg) {
    if (*cfg.portType() == cfg::PortDescriptorType::Physical) {
      return PortDescriptor(PortID(*cfg.portId()));
    }
    return PortDescriptor(AggregatePortID(*cfg.portId()));
  }
};

// helper so port descriptors work directly in folly::to<string> expressions.
inline void toAppend(const PortDescriptor& pd, std::string* result) {
  folly::toAppend(pd.str(), result);
}

} // namespace facebook::fboss
