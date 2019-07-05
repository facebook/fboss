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

#include <folly/logging/xlog.h>
#include <folly/Singleton.h>
#include <folly/io/async/EventBase.h>


#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/Constants.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmRoute.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/BcmConfig.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/platforms/test_platforms/CreateTestPlatform.h"

extern "C" {
#include <opennsl/error.h>
}

using namespace facebook::fboss;

DECLARE_bool(flexports);
DECLARE_string(bcm_config);
DECLARE_int32(thrift_port);

namespace {
void addPort(BcmConfig::ConfigMap& cfg, int port,
                      int speed, bool enabled=true) {
  auto key = folly::to<std::string>("portmap_", port);
  auto value = folly::to<std::string>(port, ":", speed);
  if (!enabled) {
    value = folly::to<std::string>(value, ":i");
  }
  cfg[key] = value;
}

void addFlexPort(BcmConfig::ConfigMap& cfg, int start, int speed) {
  addPort(cfg, start, speed);
  addPort(cfg, start + 1, speed/4, false);
  addPort(cfg, start + 2, speed/2, false);
  addPort(cfg, start + 3, speed/4, false);
}
} // namespace

namespace facebook {
namespace fboss {

BcmTest::BcmTest() {
  BcmConfig::ConfigMap cfg;
  if (getPlatformMode() == PlatformMode::FAKE_WEDGE) {
    FLAGS_flexports = true;
    for (int n = 1; n <= 125; n += 4) {
      addFlexPort(cfg, n, 40);
    }
    cfg["pbmp_xport_xe"] = "0x1fffffffffffffffffffffffe";
  } else {
    // Load from a local file
    cfg = BcmConfig::loadFromFile(FLAGS_bcm_config);
  }
  BcmAPI::init(cfg);
}

std::pair<std::unique_ptr<Platform>, std::unique_ptr<HwSwitch>>
BcmTest::createHw() const {
  auto platform = createTestPlatform();
  auto bcmSwitch = std::make_unique<BcmSwitch>(
      static_cast<BcmTestPlatform*>(platform.get()), featuresDesired());

  return std::make_pair(std::move(platform), std::move(bcmSwitch));
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

const std::vector<PortID>& BcmTest::logicalPortIds() const {
  return getPlatform()->logicalPortIds();
}

const std::vector<PortID>& BcmTest::masterLogicalPortIds() const {
  return getPlatform()->masterLogicalPortIds();
}

std::vector<PortID> BcmTest::getAllPortsinGroup(PortID portID) {
  return getPlatform()->getAllPortsinGroup(portID);
}

std::map<PortID, HwPortStats> BcmTest::getLatestPortStats(
    const std::vector<PortID>& ports) {
  auto err = opennsl_stat_sync(getHwSwitch()->getUnit());
  if (OPENNSL_FAILURE(err)) {
    XLOG(ERR) << "Unable to sync stats: " << opennsl_errmsg(err) << ", " << err;
  }
  updateHwSwitchStats(getHwSwitch());
  std::map<PortID, HwPortStats> mapPortStats;
  for (const auto& port : ports) {
    mapPortStats[port] =
        getHwSwitch()->getPortTable()->getBcmPort(port)->getPortStats();
  }
  return mapPortStats;
}

} // namespace fboss
} // namespace facebook
