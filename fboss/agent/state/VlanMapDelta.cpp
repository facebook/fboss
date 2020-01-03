/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/VlanMapDelta.h"

#include "fboss/agent/state/NodeMapDelta-defs.h"

namespace facebook::fboss {

template class NodeMapDelta<ArpTable>;
template class NodeMapDelta<NdpTable>;
template class NodeMapDelta<MacTable>;
template class NodeMapDelta<VlanMap, VlanDelta>;

} // namespace facebook::fboss
