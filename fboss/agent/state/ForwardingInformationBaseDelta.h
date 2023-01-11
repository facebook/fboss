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

#include "fboss/agent/state/ForwardingInformationBase.h"
#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NodeMapDelta.h"

namespace facebook::fboss {

class ForwardingInformationBaseContainerDelta
    : public DeltaValue<ForwardingInformationBaseContainer> {
 public:
  using DeltaValue<ForwardingInformationBaseContainer>::DeltaValue;

  thrift_cow::ThriftMapDelta<ForwardingInformationBaseV4> getV4FibDelta() const;
  thrift_cow::ThriftMapDelta<ForwardingInformationBaseV6> getV6FibDelta() const;

  template <typename AddrT>
  auto getFibDelta() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return getV4FibDelta();
    } else {
      return getV6FibDelta();
    }
  }
};

using ForwardingInformationBaseMapDelta = NodeMapDelta<
    ForwardingInformationBaseMap,
    ForwardingInformationBaseContainerDelta>;

} // namespace facebook::fboss
