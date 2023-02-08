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

#include "fboss/agent/hw/HwBasePortFb303Stats.h"
#include "fboss/agent/hw/HwFb303Stats.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"

#include "folly/container/F14Map.h"

#include <optional>
#include <string>

namespace facebook::fboss {

class HwPortFb303Stats : public HwBasePortFb303Stats {
 public:
  explicit HwPortFb303Stats(
      const std::string& portName,
      QueueId2Name queueId2Name = {})
      : HwBasePortFb303Stats(portName, queueId2Name) {
    portStats_.portName_() = portName;
    reinitStats(std::nullopt);
  }

  ~HwPortFb303Stats() override = default;
  void updateStats(
      const HwPortStats& latestStats,
      const std::chrono::seconds& retrievedAt);

  virtual void portNameChanged(const std::string& newName) override {
    portStats_.portName_() = newName;
    HwBasePortFb303Stats::portNameChanged(newName);
  }

  const HwPortStats& portStats() const {
    return portStats_;
  }

  std::chrono::seconds timeRetrieved() const {
    return timeRetrieved_;
  }

  const std::vector<folly::StringPiece>& kPortStatKeys() const override;
  const std::vector<folly::StringPiece>& kQueueStatKeys() const override;
  const std::vector<folly::StringPiece>& kInMacsecPortStatKeys() const override;
  const std::vector<folly::StringPiece>& kOutMacsecPortStatKeys()
      const override;

 private:
  HwPortStats portStats_;
  std::chrono::seconds timeRetrieved_{0};
};

} // namespace facebook::fboss
