// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/RouteUpdateWrapper.h"

namespace facebook::fboss {

class HwSwitch;
class StateDelta;

class HwSwitchRouteUpdateWrapper : public RouteUpdateWrapper {
 public:
  HwSwitchRouteUpdateWrapper(
      HwSwitch* hwSwitch,
      RoutingInformationBase* rib,
      std::function<std::shared_ptr<SwitchState>(const StateDelta&)> apply =
          nullptr);

 private:
  AdminDistance clientIdToAdminDistance(ClientID clientID) const override;
  void updateStats(
      const RoutingInformationBase::UpdateStatistics& /*stats*/) override {}

  HwSwitch* hw_;
};

} // namespace facebook::fboss
