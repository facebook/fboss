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

#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

using SflowCollectorMapTraits = NodeMapTraits<std::string, SflowCollector>;

/*
 * A container for the set of collectors.
 */
class SflowCollectorMap
    : public NodeMapT<SflowCollectorMap, SflowCollectorMapTraits> {
 public:
  SflowCollectorMap() = default;
  ~SflowCollectorMap() override = default;

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

} // namespace facebook::fboss
