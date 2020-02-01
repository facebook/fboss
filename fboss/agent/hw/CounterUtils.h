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

void deleteCounter(const std::string& oldCounterName);

} // namespace facebook::fboss::utility
