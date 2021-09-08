/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AclTableGroupMap.h"

#include "fboss/agent/state/NodeMap-defs.h"
#include "fboss/agent/state/NodeMapDelta-defs.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

AclTableGroupMap::AclTableGroupMap() {}

AclTableGroupMap::~AclTableGroupMap() {}

FBOSS_INSTANTIATE_NODE_MAP(AclTableGroupMap, AclTableGroupMapTraits);

} // namespace facebook::fboss
