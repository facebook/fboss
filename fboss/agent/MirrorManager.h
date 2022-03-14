// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/MirrorManagerImpl.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

class MirrorManager : public StateObserver {
 public:
  explicit MirrorManager(SwSwitch* sw);
  ~MirrorManager() override;

  void stateUpdated(const StateDelta& delta) override;

 private:
  SwSwitch* sw_;
  std::unique_ptr<MirrorManagerV4> v4Manager_;
  std::unique_ptr<MirrorManagerV6> v6Manager_;

  bool hasMirrorChanges(const StateDelta& delta);

  std::shared_ptr<SwitchState> resolveMirrors(
      const std::shared_ptr<SwitchState>& state);
};

} // namespace facebook::fboss
