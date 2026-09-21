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

#include <gtest/gtest.h>
#include <map>
#include <string>

namespace facebook::fboss {

class SwSwitch;

/*
 * CounterCache stores fb303 counter values, and allows
 * comparing the current values to previously recorded values.
 */
class CounterCache {
 public:
  explicit CounterCache(SwSwitch* sw) : sw_(sw) {
    update();
  }

  void update();

  void checkDelta(const std::string& name, int64_t delta) const {
#ifndef IS_OSS
    SCOPED_TRACE(name);
    auto prev = getValue(name, &prev_);
    auto cur = getValue(name, &current_);
    EXPECT_EQ(prev + delta, cur);
#else
    EXPECT_EQ(1, 1);
#endif
  }

  int64_t value(const std::string& name) const {
    return getValue(name, &current_);
  }
  int64_t prevValue(const std::string& name) const {
    return getValue(name, &prev_);
  }

  bool checkExist(const std::string& name) const {
    return current_.find(name) != current_.end();
  }

 private:
  int64_t getValue(
      const std::string& counter,
      const std::map<std::string, int64_t>* map) const {
    auto it = map->find(counter);
    if (it == map->end()) {
      return 0;
    }
    return it->second;
  }

  SwSwitch* sw_{nullptr};
  std::map<std::string, int64_t> prev_;
  std::map<std::string, int64_t> current_;
};

} // namespace facebook::fboss
