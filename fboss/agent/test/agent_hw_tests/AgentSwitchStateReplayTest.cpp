/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <folly/FileUtil.h>
#include <folly/json/dynamic.h>
#include "fboss/agent/test/AgentHwTest.h"

DEFINE_string(
    replay_switch_state_file,
    "",
    "File for dumping switch state JSON in on exit");
DECLARE_bool(disable_link_toggler);

using std::string;

namespace facebook::fboss {

class AgentSwitchStateReplayTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    if (FLAGS_replay_switch_state_file.size()) {
      // Config must be from the same switch that switch
      // state was pulled from.
      return *AgentConfig::fromDefaultFile()->thrift.sw();
      ;
    }
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(), ensemble.masterLogicalPortIds());
  }
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::HW_SWITCH};
  }
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
      } catch (const std::exception&) {
        XLOG(FATAL) << "Failed to parse replay switch state file to thrift.";
      }
    }
    // No file was given as input. This would happen when this gets
    // invoked as part of agent_test test suite. In which case, just
    // apply one of the configs for validation. We could hard fail here
    // but that would entail making replay test a stand alone tool, which
    // then forgoes benefit of continuous validation we get by being part
    // of agent_tests
    XLOG(DBG2) << "No replay state file specified, returning programmed state";
    return getProgrammedState()->clone();
  }

 protected:
  void runTest() {
    auto setup = [=, this]() {
      auto wbState = getWarmBootState();
      for (auto portMap : std::as_const(*wbState->getPorts())) {
        for (auto [_, port] : std::as_const(*portMap.second)) {
          if (port->isUp()) {
            auto portSwitchId =
                getSw()->getScopeResolver()->scope(port->getID()).switchId();
            auto portAsic = getSw()->getHwAsicTable()->getHwAsic(portSwitchId);
            auto newPort = port->modify(&wbState);
            newPort->setLoopbackMode(
                portAsic->getDesiredLoopbackMode(newPort->getPortType()));
            XLOG(DBG2) << " Setting loopback mode for : " << newPort->getID();
          }
        }
      }
      applyNewState([wbState](const std::shared_ptr<SwitchState>& /*in*/) {
        return wbState;
      });
    };
    auto verify = [=]() {};
    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // All ports should be visible like in prod
    FLAGS_hide_fabric_ports = false;
    // Don't start with bringing up all ports via toggler
    FLAGS_disable_link_toggler = true;
  }
};

TEST_F(AgentSwitchStateReplayTest, test) {
  runTest();
}

} // namespace facebook::fboss
