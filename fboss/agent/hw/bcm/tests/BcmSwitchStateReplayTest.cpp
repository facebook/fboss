/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/FileUtil.h>
#include <folly/dynamic.h>

DEFINE_string(
    replay_switch_state_file,
    "",
    "File for dumping switch state JSON in on exit");
using std::string;

namespace facebook::fboss {

class BcmSwitchStateReplayTest : public BcmTest {
  std::shared_ptr<SwitchState> getWarmBootState() const {
    if (FLAGS_replay_switch_state_file.size()) {
      std::string warmBootJson;
      auto ret =
          folly::readFile(FLAGS_replay_switch_state_file.c_str(), warmBootJson);
      sysCheckError(
          ret,
          "Unable to read switch state from : ",
          FLAGS_replay_switch_state_file);
      return SwitchState::fromFollyDynamic(
          folly::parseJson(warmBootJson)["swSwitch"]);
    }
    // No file was given as input. This would happen when this gets
    // invoked as part of bcm_test test suite. In which case, just
    // apply one of the configs for validation. We could hard fail here
    // but that would entail making replay test a stand alone tool, which
    // then forgoes benefit of continuous validation we get by being part
    // of bcm_tests
    XLOG(INFO)
        << "No replay state file specified, applying one port per vlan config";
    auto config =
        utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
    return applyThriftConfig(getProgrammedState(), &config, getPlatform());
  }

 protected:
  void runTest() {
    auto setup = [=]() {
      auto wbState = getWarmBootState();
      for (auto port : *wbState->getPorts()) {
        if (port->isUp()) {
          port->setLoopbackMode(cfg::PortLoopbackMode::MAC);
          XLOG(INFO) << " Setting loopback mode for : " << port->getID();
        }
      }
      applyNewState(wbState);
    };
    auto verify = [=]() {};
    verifyAcrossWarmBoots(setup, verify);
  }

 private:
};

TEST_F(BcmSwitchStateReplayTest, test) {
  runTest();
}

} // namespace facebook::fboss
