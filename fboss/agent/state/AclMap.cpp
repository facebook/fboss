/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AclMap.h"

#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/NodeMap-defs.h"

namespace facebook { namespace fboss {

AclMap::AclMap() {
}

AclMap::~AclMap() {
}

FBOSS_INSTANTIATE_NODE_MAP(AclMap, AclMapTraits);

}} // facebook::fboss
