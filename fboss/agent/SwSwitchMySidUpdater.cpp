// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchMySidUpdater.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/rib/MySidMapUpdater.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

RibMySidToSwitchStateFunction createRibMySidToSwitchStateFunction(
    const std::optional<StateDeltaApplication>& deltaApplicationBehavior) {
  return [deltaApplicationBehavior](
             const facebook::fboss::SwitchIdScopeResolver* resolver,
             const MySidTable& mySidTable,
             void* cookie) -> StateDelta {
    facebook::fboss::MySidMapUpdater mySidMapUpdater(resolver, mySidTable);
    auto sw = static_cast<facebook::fboss::SwSwitch*>(cookie);

    auto oldState = sw->getState();

    sw->updateStateWithHwFailureProtection(
        "update mySids",
        [&mySidMapUpdater](const std::shared_ptr<SwitchState>& in) {
          return mySidMapUpdater(in);
        },
        deltaApplicationBehavior);

    return StateDelta(oldState, sw->getState());
  };
}

} // namespace facebook::fboss
