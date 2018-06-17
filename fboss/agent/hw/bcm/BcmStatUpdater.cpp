/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmAclStat.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"

#include <boost/container/flat_map.hpp>

namespace facebook { namespace fboss {

using std::chrono::duration_cast;
using std::chrono::seconds;
using std::chrono::system_clock;

BcmStatUpdater::BcmStatUpdater(int unit)
  : unit_(unit) {}

void BcmStatUpdater::toBeAddedAclStat(BcmAclStatHandle handle,
  const std::string& name) {
  toBeAddedAclStats_.emplace(handle, name);
}

void BcmStatUpdater::toBeRemovedAclStat(BcmAclStatHandle handle) {
  toBeRemovedAclStats_.emplace(handle);
}

void BcmStatUpdater::refresh() {
  if (toBeRemovedAclStats_.empty() && toBeAddedAclStats_.empty()) {
    return;
  }

  auto lockedAclStats = aclStats_.wlock();

  while (!toBeRemovedAclStats_.empty()) {
    BcmAclStatHandle handle = toBeRemovedAclStats_.front();
    auto erased = lockedAclStats->erase(handle);
    if (!erased) {
      throw FbossError("Trying to remove non-existent, handle=", handle);
    }
    toBeRemovedAclStats_.pop();
  }

  while (!toBeAddedAclStats_.empty()) {
    BcmAclStatHandle handle = toBeAddedAclStats_.front().first;
    const std::string& name = toBeAddedAclStats_.front().second;
    auto inserted = lockedAclStats->emplace(handle,
      std::make_unique<MonotonicCounter>(name, stats::SUM, stats::RATE));
    if (!inserted.second) {
      throw FbossError("Duplicate ACL stat handle, handle=", handle,
                       ", name=", name);
    }
    toBeAddedAclStats_.pop();
  }
}

void BcmStatUpdater::updateStats() {
  auto now = duration_cast<seconds>(system_clock::now().time_since_epoch());
  auto lockedAclStats = aclStats_.wlock();
  for (auto& entry : *lockedAclStats) {
    updateAclStat(unit_, entry.first, now, entry.second.get());
  }
}

size_t BcmStatUpdater::getCounterCount() const {
  return aclStats_.rlock()->size();
}

MonotonicCounter* FOLLY_NULLABLE BcmStatUpdater::getCounterIf(
  BcmAclStatHandle handle) {
  auto lockedAclStats = aclStats_.rlock();
  auto iter = lockedAclStats->find(handle);
  if (iter == lockedAclStats->end()) {
      return nullptr;
  }
  return iter->second.get();
}

}} // facebook::fboss
