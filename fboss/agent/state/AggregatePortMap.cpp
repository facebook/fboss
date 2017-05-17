/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/AggregatePortMap.h"

#include "fboss/agent/state/NodeMap-defs.h"

namespace facebook { namespace fboss {

AggregatePortMap::AggregatePortMap() {}

AggregatePortMap::~AggregatePortMap() {}

FBOSS_INSTANTIATE_NODE_MAP(AggregatePortMap, AggregatePortMapTraits);

}} // facebook::fboss
