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

#include <folly/Range.h>
#include <chrono>

namespace facebook {

class SpinLock {
};

class SpinLockHolder {
public:
  explicit SpinLockHolder(SpinLock*) {}
};

namespace stats {

enum ExportType {
  SUM,
  COUNT,
  AVG,
  RATE,
  PERCENT,
  NUM_TYPES,
};

struct ExportedStat {
  void addValue(std::chrono::seconds, int64_t) {}
  int getSum(int level) {return 0;}
};

class ExportedStatMap {
public:
  class LockAndStatItem {
  public:
    std::shared_ptr<SpinLock> first;
    std::shared_ptr<ExportedStat> second;
  };
  LockAndStatItem getLockAndStatItem(folly::StringPiece,
                                     const ExportType* = nullptr) {
    static LockAndStatItem it = {
      std::make_shared<SpinLock>(), std::make_shared<ExportedStat>()
    };
    return it;
  }

  std::shared_ptr<ExportedStat> getStatPtr(folly::StringPiece name) {
      return std::make_shared<ExportedStat>();
  }
};

}}
