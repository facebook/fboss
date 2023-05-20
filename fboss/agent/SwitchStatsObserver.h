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
  SwSwitch* sw_;
};

} // namespace facebook::fboss
