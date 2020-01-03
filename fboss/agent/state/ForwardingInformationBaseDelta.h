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

  NodeMapDelta<ForwardingInformationBaseV4> getV4FibDelta() const;
  NodeMapDelta<ForwardingInformationBaseV6> getV6FibDelta() const;
};

using ForwardingInformationBaseMapDelta = NodeMapDelta<
    ForwardingInformationBaseMap,
    ForwardingInformationBaseContainerDelta>;

} // namespace facebook::fboss
