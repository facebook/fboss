/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/PortMap.h"

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/NodeMap-defs.h"

namespace facebook { namespace fboss {

PortMap::PortMap() {
}

PortMap::~PortMap() {
}

void PortMap::registerPort(PortID id, const std::string& name) {
  addNode(std::make_shared<Port>(id, name));
}

FBOSS_INSTANTIATE_NODE_MAP(PortMap, PortMapTraits);

}} // facebook::fboss
