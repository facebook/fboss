// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/rib/RouteUpdater.h"
#include "fboss/agent/state/MySidMap.h"

#include <memory>

namespace facebook::fboss {

class SwitchState;
class SwitchIdScopeResolver;

class MySidMapUpdater {
 public:
  MySidMapUpdater(
      const SwitchIdScopeResolver* resolver,
      const MySidTable& mySidTable);

  std::shared_ptr<SwitchState> operator()(
      const std::shared_ptr<SwitchState>& state);

 private:
  std::shared_ptr<MultiSwitchMySidMap> createUpdatedMySidMap(
      const MySidTable& ribMySidTable,
      const std::shared_ptr<MultiSwitchMySidMap>& currentMySids);

  const SwitchIdScopeResolver* resolver_;
  const MySidTable& mySidTable_;
};

} // namespace facebook::fboss
