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

#include "fboss/agent/state/ForwardingInformationBaseContainer.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/types.h"

namespace facebook {
namespace fboss {

using ForwardingInformationBaseMapTraits =
    NodeMapTraits<RouterID, ForwardingInformationBaseContainer>;

class ForwardingInformationBaseMap : public NodeMapT<
                                         ForwardingInformationBaseMap,
                                         ForwardingInformationBaseMapTraits> {
 public:
  ForwardingInformationBaseMap();
  ~ForwardingInformationBaseMap() override;

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace fboss
} // namespace facebook
