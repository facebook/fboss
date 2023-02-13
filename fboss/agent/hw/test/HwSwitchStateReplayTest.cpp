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
#include "fboss/agent/hw/test/HwTest.h"

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

class HwSwitchStateReplayTest : public HwTest {
  std::shared_ptr<SwitchState> getWarmBootState() {
    if (FLAGS_replay_switch_state_file.size()) {
      std::vector<std::byte> bytes;
      auto ret = folly::readFile(FLAGS_replay_switch_state_file.c_str(), bytes);
      sysCheckError(
          ret,
          "Unable to read switch state from : ",
          FLAGS_replay_switch_state_file);
      // By default parse switch state as thrift
      state::WarmbootState thriftState;
      auto buf = folly::IOBuf::copyBuffer(bytes.data(), bytes.size());
      apache::thrift::BinaryProtocolReader reader;
      reader.setInput(buf.get());
      try {
        thriftState.read(&reader);
        return SwitchState::fromThrift(*thriftState.swSwitchState());
      } catch (const std::exception& e) {
        XLOG(FATAL) << "Failed to parse replay switch state file to thrift.";
      }
    }
    // No file was given as input. This would happen when this gets
    // invoked as part of bcm_test test suite. In which case, just
    // apply one of the configs for validation. We could hard fail here
    // but that would entail making replay test a stand alone tool, which
    // then forgoes benefit of continuous validation we get by being part
    // of bcm_tests
    XLOG(DBG2)
        << "No replay state file specified, applying one port per vlan config";
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
    applyNewConfig(config);
    return getProgrammedState();
  }

 protected:
  void runTest() {
    auto setup = [=]() {
      auto wbState = getWarmBootState();
      for (auto port : std::as_const(*wbState->getPorts())) {
        if (port.second->isUp()) {
          port.second->setLoopbackMode(getAsic()->desiredLoopbackMode());
          XLOG(DBG2) << " Setting loopback mode for : " << port.second->getID();
        }
      }
      applyNewState(wbState);
    };
    auto verify = [=]() {};
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwSwitchStateReplayTest, test) {
  runTest();
}

} // namespace facebook::fboss
