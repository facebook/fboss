// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/RouteUpdateWrapper.h"

namespace facebook::fboss {

class HwSwitch;
class HwSwitchRouteUpdateWrapper : public RouteUpdateWrapper {
 public:
  HwSwitchRouteUpdateWrapper(
      HwSwitch* hwSwitch,
      RoutingInformationBase* rib,
      FibUpdateFunction fibUpdateFunction);

 private:
  AdminDistance clientIdToAdminDistance(ClientID clientID) const override;
  void updateStats(
      const RoutingInformationBase::UpdateStatistics& /*stats*/) override {}

  HwSwitch* hw_;
};

} // namespace facebook::fboss
