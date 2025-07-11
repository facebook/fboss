// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/PreUpdateStateModifier.h"

namespace facebook::fboss {
class StateDelta;
class SwitchState;

class ShelManager : public PreUpdateStateModifier {
 public:
  std::vector<StateDelta> modifyState(
      const std::vector<StateDelta>& deltas) override;
  void updateDone() override;
  void updateFailed(const std::shared_ptr<SwitchState>& curState) override;

 private:
};

} // namespace facebook::fboss
