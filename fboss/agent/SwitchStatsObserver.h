// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/StateObserver.h"

namespace facebook::fboss {
class SwSwitch;
class SwitchState;

class SwitchStatsObserver : public StateObserver {
 public:
  explicit SwitchStatsObserver(SwSwitch* sw);
  ~SwitchStatsObserver() override;
  void stateUpdated(const StateDelta& delta) override;

 private:
  template <typename NTableT>
  int getResolvedNeighborCount(const NTableT& ntable);
  template <typename IntfDeltaT>
  std::tuple<int, int, int> getUpdatedInterfaceCounters(
      const IntfDeltaT& intfDelta);
  void updateInterfaceCounters(const StateDelta& delta);
  void updateSystemPortCounters(const StateDelta& delta);

  SwSwitch* sw_;
};

} // namespace facebook::fboss
