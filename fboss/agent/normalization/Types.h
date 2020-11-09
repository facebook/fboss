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

#include "fboss/agent/types.h"

namespace facebook::fboss::normalization {
/*
 * Define data structures that are commonly referenced across the module
 */

// type of transforms supported
enum class TransformType {
  RATE,
  BPS,
};

// a counter representation
struct Counter {
  Counter() = default;
  Counter(StatTimestamp timestamp, int64_t value)
      : timestamp(timestamp), value(value) {}

  StatTimestamp timestamp;
  int64_t value;
};

} // namespace facebook::fboss::normalization
