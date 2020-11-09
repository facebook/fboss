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

#include <string>

namespace facebook::fboss::normalization {

// abstraction for stats exporting logic
class StatsExporter {
 public:
  virtual void publishPortStats(
      const std::string& portName,
      const std::string& propertyName,
      int64_t timestamp,
      double value) = 0;

  virtual ~StatsExporter() = default;
};

} // namespace facebook::fboss::normalization
