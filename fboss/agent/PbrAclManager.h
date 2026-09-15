// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <vector>

namespace facebook::fboss {
class StateDelta;
class SwitchState;

class PbrAclManager {
 public:
  std::vector<StateDelta> modifyState(const std::vector<StateDelta>& deltas);
  std::vector<StateDelta> reconstructFromSwitchState(
      const std::shared_ptr<SwitchState>& curState);
  void updateDone();
  void updateFailed(const std::shared_ptr<SwitchState>& curState);

 private:
  std::shared_ptr<SwitchState> processDelta(const StateDelta& delta);
  std::vector<StateDelta> modifyStateImpl(
      const std::vector<StateDelta>& deltas);
};

} // namespace facebook::fboss
