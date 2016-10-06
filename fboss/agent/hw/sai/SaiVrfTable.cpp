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
#include "SaiVrfTable.h"
#include "SaiVrf.h"
#include "SaiSwitch.h"
#include "SaiError.h"

namespace facebook { namespace fboss {

SaiVrfTable::SaiVrfTable(const SaiSwitch *hw) : hw_(hw) {
  VLOG(4) << "Entering " << __FUNCTION__;
}

SaiVrfTable::~SaiVrfTable() {
  VLOG(4) << "Entering " << __FUNCTION__;
}

sai_object_id_t SaiVrfTable::getSaiVrfId(RouterID fbossVrfId) const {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto iter = vrfs_.find(fbossVrfId);

  if (iter == vrfs_.end()) {
    throw SaiError("Cannot find SaiVrf object for vrf: ", fbossVrfId);
  }

  return iter->second.first->getSaiVrfId();
}

SaiVrf* SaiVrfTable::incRefOrCreateSaiVrf(RouterID fbossVrfId) {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto ret = vrfs_.emplace(fbossVrfId, std::make_pair(nullptr, 1));
  auto &iter = ret.first;

  if (!ret.second) {
    // there was an entry already in the map
    iter->second.second++;  // increase the reference counter
    return iter->second.first.get();
  }

  SCOPE_FAIL {
    vrfs_.erase(iter);
  };

  auto newVrf = folly::make_unique<SaiVrf>(hw_, fbossVrfId);
  auto vrfPtr = newVrf.get();
  iter->second.first = std::move(newVrf);

  return vrfPtr;
}

SaiVrf* SaiVrfTable::derefSaiVrf(RouterID fbossVrfId) noexcept {
  VLOG(4) << "Entering " << __FUNCTION__;

  auto iter = vrfs_.find(fbossVrfId);

  if (iter == vrfs_.end()) {
    return nullptr;
  }

  auto &entry = iter->second;
  CHECK_GT(entry.second, 0);

  if (--entry.second == 0) {
    vrfs_.erase(iter);
    return nullptr;
  }

  return entry.first.get();
}

}} // facebook::fboss
