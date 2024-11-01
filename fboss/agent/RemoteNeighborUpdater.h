// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/StateObserver.h"

namespace facebook::fboss {

class SwSwitch;

class RemoteNeighborUpdater : public StateObserver {
 public:
  explicit RemoteNeighborUpdater(SwSwitch* sw);
  virtual ~RemoteNeighborUpdater() override {}
  void stateUpdated(const StateDelta& delta) override;

 private:
  void processChangedInterface(
      std::shared_ptr<SwitchState>* state,
      const std::shared_ptr<Interface>& newInterface) const;
  SwSwitch* sw_{nullptr};
};

} // namespace facebook::fboss
