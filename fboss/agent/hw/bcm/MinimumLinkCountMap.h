/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <boost/container/flat_map.hpp>
#include <glog/logging.h>

#include <utility>

#include <folly/SharedMutex.h>
#include <optional>

namespace facebook::fboss {

class TrunkToMinimumLinkCountMap {
 public:
  TrunkToMinimumLinkCountMap() : trunkToCountLock_(), trunkToCount_() {}

  void addOrUpdate(bcm_trunk_t trunk, uint8_t count) {
    folly::SharedMutexReadPriority::WriteHolder g(&trunkToCountLock_);
    addLocked(trunk, count);
  }

  void del(bcm_trunk_t trunk) {
    folly::SharedMutexReadPriority::WriteHolder g(&trunkToCountLock_);
    delLocked(trunk);
  }

  std::optional<uint8_t> get(bcm_trunk_t trunk) const {
    folly::SharedMutexReadPriority::ReadHolder g(&trunkToCountLock_);
    return getLocked(trunk);
  }

 private:
  void addLocked(bcm_trunk_t trunk, uint8_t count) {
    bool inserted;
    boost::container::flat_map<bcm_trunk_t, uint8_t>::iterator it;
    std::tie(it, inserted) = trunkToCount_.emplace(trunk, count);
    if (!inserted) {
      it->second = count;
    }
  }

  void delLocked(bcm_trunk_t trunk) {
    auto it = trunkToCount_.find(trunk);
    CHECK(it != trunkToCount_.end());
    trunkToCount_.erase(it);
  }

  std::optional<uint8_t> getLocked(bcm_trunk_t trunk) const {
    auto it = trunkToCount_.find(trunk);
    return it == trunkToCount_.cend() ? std::nullopt
                                      : std::make_optional(it->second);
  }

  mutable folly::SharedMutexReadPriority trunkToCountLock_;
  boost::container::flat_map<bcm_trunk_t, uint8_t> trunkToCount_;
};

} // namespace facebook::fboss
