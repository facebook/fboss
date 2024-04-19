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

#include "fboss/agent/StateObserver.h"

namespace facebook::fboss {

class StateDelta;
class SwSwitch;

class PortUpdateHandler : public StateObserver {
 public:
  explicit PortUpdateHandler(SwSwitch* sw);
  ~PortUpdateHandler();

  void stateUpdated(const StateDelta& delta) override;

 private:
  void disableIfLooped(
      const std::shared_ptr<Port>& newPort,
      const std::shared_ptr<SwitchState>& newState);
  void clearErrorDisableLoopDetected(
      const std::shared_ptr<Port>& newPort,
      const std::shared_ptr<SwitchState>& newState);
  void checkNewlyUndrained(const StateDelta& delta);
  // Forbidden copy constructor and assignment operator
  PortUpdateHandler(PortUpdateHandler const&) = delete;
  PortUpdateHandler& operator=(PortUpdateHandler const&) = delete;
  void computeFabricOverdrainPct(const StateDelta& delta);

  SwSwitch* sw_{nullptr};
};

} // namespace facebook::fboss
