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

#include "fboss/agent/state/PortDescriptorTemplate.h"

#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

class PortDescriptor : public PortDescriptorTemplate<PortID, AggregatePortID> {
 public:
  using BaseT = PortDescriptorTemplate<PortID, AggregatePortID>;
  explicit PortDescriptor(PortID p) : BaseT(p) {}
  explicit PortDescriptor(AggregatePortID p) : BaseT(p) {}

  static PortDescriptor fromRxPacket(const RxPacket& pkt) {
    if (pkt.isFromAggregatePort()) {
      return PortDescriptor(AggregatePortID(pkt.getSrcAggregatePort()));
    } else {
      return PortDescriptor(PortID(pkt.getSrcPort()));
    }
  }

  static PortDescriptor fromFollyDynamic(const folly::dynamic& descJSON) {
    return PortDescriptor(BaseT::fromFollyDynamic(descJSON));
  }

 private:
  explicit PortDescriptor(const BaseT& b) : BaseT(b) {}
};

// helper so port descriptors work directly in folly::to<string> expressions.
inline void toAppend(const PortDescriptor& pd, std::string* result) {
  folly::toAppend(pd.str(), result);
}

} // namespace facebook::fboss
