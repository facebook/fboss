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

#include <cstdint>
#include <string>
#include <vector>

#include <fb303/ExportType.h>
#include <fb303/ExportedStatMapImpl.h>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "folly/Range.h"

namespace facebook::fboss::utility {

class CounterPrevAndCur {
 public:
  CounterPrevAndCur(int64_t _prev, int64_t _cur);
  int64_t incrementFromPrev() const;

 private:
  bool rolledOver() const {
    return cur_ < prev_;
  }
  bool curUninitialized() const {
    return counterUninitialized(cur_);
  }
  bool prevUninitialized() const {
    return counterUninitialized(prev_);
  }
  bool anyUninitialized() const {
    return curUninitialized() || prevUninitialized();
  }

  bool counterUninitialized(const int64_t& val) const;
  int64_t prev_;
  int64_t cur_;
};

int64_t subtractIncrements(
    const CounterPrevAndCur& counterRaw,
    const std::vector<CounterPrevAndCur>& countersToSubtract);

void deleteCounter(const folly::StringPiece oldCounterName);

std::string counterTypeToString(cfg::CounterType type);
std::string statNameFromCounterType(
    const std::string& statPrefix,
    cfg::CounterType counterType,
    std::optional<std::string> mutiSwitchPrefix = std::nullopt);

class ExportedCounter {
 public:
  ExportedCounter(
      const folly::StringPiece name,
      const folly::Range<const fb303::ExportType*>& exportTypes);
  ~ExportedCounter(void);
  ExportedCounter() = delete;
  ExportedCounter(const ExportedCounter&) = delete;
  ExportedCounter& operator=(const ExportedCounter&) = delete;
  ExportedCounter(ExportedCounter&& other) noexcept;
  ExportedCounter& operator=(ExportedCounter&& other);

  void setName(const folly::StringPiece name);

  void incrementValue(std::chrono::seconds now, int64_t amount = 1);

 private:
  void exportStat(const folly::StringPiece name);
  void unexport();

  fb303::ExportedStatMapImpl::LockableStat stat_;
  std::string name_;
  folly::Range<const fb303::ExportType*> exportTypes_;
};

} // namespace facebook::fboss::utility
