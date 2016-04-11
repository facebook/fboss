/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#include "fboss/agent/state/RouteForwardInfo.h"

#include "SaiNextHopTable.h"
#include "SaiNextHop.h"
#include "SaiSwitch.h"
#include "SaiError.h"

namespace facebook { namespace fboss {

SaiNextHopTable::SaiNextHopTable(const SaiSwitch *hw) : hw_(hw) {
  VLOG(4) << "Entering " << __FUNCTION__;
}

SaiNextHopTable::~SaiNextHopTable() {
  VLOG(4) << "Entering " << __FUNCTION__;
}

sai_object_id_t SaiNextHopTable::GetSaiNextHopId(const RouteForwardInfo &fwdInfo) const {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto iter = nextHops_.find(fwdInfo.getNexthops());

  if (iter == nextHops_.end()) {
    return SAI_NULL_OBJECT_ID;
  }

  return iter->second.first->GetSaiNextHopId();
}

SaiNextHop* SaiNextHopTable::IncRefOrCreateSaiNextHop(const RouteForwardInfo &fwdInfo) {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto ret = nextHops_.emplace(fwdInfo.getNexthops(), std::make_pair(nullptr, 1));
  auto &iter = ret.first;

  if (!ret.second) {
    // there was an entry already in the map
    iter->second.second++;  // increase the reference counter
    return iter->second.first.get();
  }

  SCOPE_FAIL {
    nextHops_.erase(iter);
  };

  auto newNextHop = folly::make_unique<SaiNextHop>(hw_, fwdInfo);
  newNextHop->Program();
  auto nextHopPtr = newNextHop.get();
  iter->second.first = std::move(newNextHop);

  return nextHopPtr;
}

SaiNextHop* SaiNextHopTable::DerefSaiNextHop(const RouteForwardInfo &fwdInfo) noexcept {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto iter = nextHops_.find(fwdInfo.getNexthops());

  if (iter == nextHops_.end()) {
    return nullptr;
  }

  auto &entry = iter->second;
  CHECK_GT(entry.second, 0);

  if (--entry.second == 0) {
    nextHops_.erase(iter);
    return nullptr;
  }

  return entry.first.get();
}

void SaiNextHopTable::onResolved(InterfaceID intf, const folly::IPAddress &ip) {

  for (auto iter = nextHops_.begin(); iter != nextHops_.end(); ++iter) {
    try {
      iter->second.first->onResolved(intf, ip);
    } catch (const SaiError &e) {
      LOG(ERROR) << e.what();
    }
  }
}

}} // facebook::fboss
