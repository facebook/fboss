/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/NodeMap-defs.h"

namespace facebook {
namespace fboss {

ForwardingInformationBaseMap::ForwardingInformationBaseMap() {}

ForwardingInformationBaseMap::~ForwardingInformationBaseMap() {}

std::pair<uint64_t, uint64_t> ForwardingInformationBaseMap::getRouteCount()
    const {
  uint64_t v4Count = 0;
  uint64_t v6Count = 0;

  for (const auto& fibContainer : *this) {
    v4Count += fibContainer->getFibV4()->size();
    v6Count += fibContainer->getFibV6()->size();
  }

  return std::make_pair(v4Count, v6Count);
}

FBOSS_INSTANTIATE_NODE_MAP(
    ForwardingInformationBaseMap,
    ForwardingInformationBaseMapTraits);

} // namespace fboss
} // namespace facebook
