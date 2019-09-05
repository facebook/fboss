/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include <folly/Singleton.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

extern "C" {
#include <opennsl/error.h>
}

DECLARE_int32(thrift_port);


namespace facebook {
namespace fboss {

BcmTest::BcmTest() {
}

BcmTest::~BcmTest() {}

std::unique_ptr<HwSwitchEnsemble> BcmTest::createHw() const {
  return std::make_unique<BcmSwitchEnsemble>(featuresDesired());
}

folly::dynamic BcmTest::createWarmBootSwitchState() {
  folly::dynamic state = folly::dynamic::object;
  state[kSwSwitch] = getProgrammedState()->toFollyDynamic();
  state[kHwSwitch] = getHwSwitch()->toFollyDynamic();
  return state;
}

std::list<FlexPortMode> BcmTest::getSupportedFlexPortModes() {
  return getPlatform()->getSupportedFlexPortModes();
}

int BcmTest::getUnit() const {
  return getHwSwitch()->getUnit();
}

std::map<PortID, HwPortStats> BcmTest::getLatestPortStats(
    const std::vector<PortID>& ports) {
  auto rv = opennsl_stat_sync(getHwSwitch()->getUnit());
  bcmCheckError(rv, "Unable to sync stats ");
  updateHwSwitchStats(getHwSwitch());
  std::map<PortID, HwPortStats> mapPortStats;
  for (const auto& port : ports) {
    auto stats =
        getHwSwitch()->getPortTable()->getBcmPort(port)->getPortStats();
    mapPortStats[port] = (stats) ? *stats : HwPortStats{};
  }
  return mapPortStats;
}

void BcmTest::collectTestFailureInfo() const {
  getHwSwitch()->printDiagCmd("show c");
}
} // namespace fboss
} // namespace facebook
