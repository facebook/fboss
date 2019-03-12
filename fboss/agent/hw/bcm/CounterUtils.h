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
#include <vector>


namespace facebook {
namespace fboss {
namespace utility {

struct CounterPrevAndCur {
  CounterPrevAndCur(int64_t _prev, int64_t _cur) : prev(_prev), cur(_cur) {}
  bool rolledOver() const {
    return cur < prev;
  }
  bool curUninitialized() const {
    return counterUninitialized(cur);
  }
  bool prevUninitialized() const {
    return counterUninitialized(prev);
  }

 private:
  bool counterUninitialized(const int64_t& val) const;
 public:
  int64_t prev;
  int64_t cur;
};

int64_t getDerivedCounterIncrement(
    const CounterPrevAndCur& counterRaw,
    const std::vector<CounterPrevAndCur>& countersToSubtract);

int64_t getDerivedCounterIncrement(
    int64_t counterRawPrev,
    int64_t counterRawCur,
    int64_t counterToSubPrev,
    int64_t counterToSubCur);
}
} // namespace fboss
} // namespace facebook
