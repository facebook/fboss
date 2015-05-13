/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/SfpMap.h"
#include <folly/Memory.h>
#include "fboss/agent/FbossError.h"

namespace facebook { namespace fboss {

SfpMap::SfpMap() {
}

SfpMap::~SfpMap() {
}

SfpModule* SfpMap::sfpModule(PortID portID) const{
  auto it = sfpMap_.find(portID);
  if (it == sfpMap_.end()) {
    throw FbossError("No sfp entry found for port");
  }
  return it->second.get();
}

void SfpMap::createSfp(PortID portID, std::unique_ptr<SfpModule> sfpModule) {
  auto rv = sfpMap_.emplace(std::make_pair(portID, std::move(sfpModule)));
  const auto& it = rv.first;
  DCHECK(rv.second);
}

PortSfpMap::const_iterator SfpMap::begin() const {
  return sfpMap_.begin();
}

PortSfpMap::const_iterator SfpMap::end() const {
  return sfpMap_.end();
}

}} // facebook::fboss
