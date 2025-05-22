// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
//
#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchFullScaleDsfTests.h"

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

DECLARE_int32(ecmp_resource_percentage);

namespace facebook::fboss {

class AgentVoqSwitchFullScaleDsfNodesWithFabricPortsTest
    : public AgentVoqSwitchFullScaleDsfNodesTest {
 private:
  void setCmdLineFlagOverrides() const override {
    AgentVoqSwitchFullScaleDsfNodesTest::setCmdLineFlagOverrides();
    // Unhide fabric ports
    FLAGS_hide_fabric_ports = false;
    // Fabric connectivity manager to expect single NPU
    if (!FLAGS_multi_switch) {
      FLAGS_janga_single_npu_for_testing = true;
    }
  }
};

TEST_F(AgentVoqSwitchFullScaleDsfNodesTest, systemPortScaleTest) {
  auto setup = [this]() {
    utility::setupRemoteIntfAndSysPorts(
        getSw(),
        isSupportedOnAllAsics(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(
    AgentVoqSwitchFullScaleDsfNodesWithFabricPortsTest,
    failUpdateAtFullSysPortScale) {
  auto setup = [this]() {
    utility::setupRemoteIntfAndSysPorts(
        getSw(),
        isSupportedOnAllAsics(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
  };
  auto verify = [this]() {
    getSw()->getRib()->updateStateInRibThread([this]() {
      EXPECT_THROW(
          getSw()->updateStateWithHwFailureProtection(
              "Add bogus intf",
              [this](const std::shared_ptr<SwitchState>& in) {
                auto out = in->clone();
                auto remoteIntfs = out->getRemoteInterfaces()->modify(&out);
                auto remoteIntf = std::make_shared<Interface>(
                    InterfaceID(INT32_MAX),
                    RouterID(0),
                    std::optional<VlanID>(std::nullopt),
                    folly::StringPiece("RemoteIntf"),
                    folly::MacAddress("c6:ca:2b:2a:b1:b6"),
                    9000,
                    false,
                    false,
                    cfg::InterfaceType::SYSTEM_PORT);
                remoteIntf->setScope(cfg::Scope::GLOBAL);
                remoteIntfs->addNode(
                    remoteIntf,
                    HwSwitchMatcher(
                        getSw()->getSwitchInfoTable().getL3SwitchIDs()));
                return out;
              }),
          FbossHwUpdateError);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
