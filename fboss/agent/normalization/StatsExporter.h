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

#include <memory>
#include <string>
#include <vector>

#include <gflags/gflags.h>

DECLARE_string(normalized_counter_entity_prefix);

namespace facebook::fboss::normalization {

// abstraction for stats exporting logic
class StatsExporter {
 public:
  explicit StatsExporter(const std::string& deviceName)
      : deviceName_(deviceName) {}

  virtual void publishPortStats(
      const std::string& portName,
      const std::string& propertyName,
      int64_t timestamp,
      double value,
      int32_t intervalSec,
      std::shared_ptr<std::vector<std::string>> tags);

  // flush counters downstream. Note the counters published will be
  // buffered locally before calling this method
  virtual void flushCounters() = 0;

  struct OdsCounter {
    std::string entity;
    std::string key;
    int64_t unixTime;
    double value;
    int32_t intervalSec;
    std::shared_ptr<std::vector<std::string>> tags;
  };

  const std::vector<OdsCounter>& getCounterBuffer() const {
    return counterBuffer_;
  }

  virtual ~StatsExporter() = default;

 protected:
  std::string deviceName_;
  // buffer counters so that we can publish to ODS in batch
  std::vector<OdsCounter> counterBuffer_;
};

// just log ODS counters values without writing to ODS
class GlogStatsExporter : public StatsExporter {
 public:
  explicit GlogStatsExporter(const std::string& deviceName)
      : StatsExporter(deviceName) {}

  void flushCounters() override;
};

} // namespace facebook::fboss::normalization
