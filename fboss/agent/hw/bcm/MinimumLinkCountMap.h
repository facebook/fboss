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
#include <utility>

#include <folly/Optional.h>
#include <folly/SharedMutex.h>

namespace facebook {
namespace fboss {

class TrunkToMinimumLinkCountMap {
 public:
  TrunkToMinimumLinkCountMap() : trunkToCountLock_(), trunkToCount_() {}

  void addOrUpdate(opennsl_trunk_t trunk, uint8_t count) {
    folly::SharedMutexReadPriority::WriteHolder g(&trunkToCountLock_);
    addLocked(trunk, count);
  }

  void del(opennsl_trunk_t trunk) {
    folly::SharedMutexReadPriority::WriteHolder g(&trunkToCountLock_);
    delLocked(trunk);
  }

  folly::Optional<uint8_t> get(opennsl_trunk_t trunk) const {
    folly::SharedMutexReadPriority::ReadHolder g(&trunkToCountLock_);
    return getLocked(trunk);
  }

 private:
  void addLocked(opennsl_trunk_t trunk, uint8_t count) {
    bool inserted;
    boost::container::flat_map<opennsl_trunk_t, uint8_t>::iterator it;
    std::tie(it, inserted) = trunkToCount_.emplace(trunk, count);
    if (!inserted) {
      it->second = count;
    }
  }

  void delLocked(opennsl_trunk_t trunk) {
    auto it = trunkToCount_.find(trunk);
    CHECK(it != trunkToCount_.end());
    trunkToCount_.erase(it);
  }

  folly::Optional<uint8_t> getLocked(opennsl_trunk_t trunk) const {
    auto it = trunkToCount_.find(trunk);
    return it == trunkToCount_.cend() ? folly::none
                                      : folly::make_optional(it->second);
  }

  mutable folly::SharedMutexReadPriority trunkToCountLock_;
  boost::container::flat_map<opennsl_trunk_t, uint8_t> trunkToCount_;
};
}
}
