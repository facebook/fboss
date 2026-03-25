// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
//
#include "fboss/agent/test/agent_hw_tests/AgentVoqSwitchFullScaleDsfTests.h"

#include "fboss/agent/FabricConnectivityManager.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

DECLARE_int32(ecmp_resource_percentage);

namespace facebook::fboss {

TEST_F(AgentVoqSwitchFullScaleDsfNodesTest, systemPortScaleTest) {
  auto setup = [this]() {
    utility::setupRemoteIntfAndSysPorts(
        getSw(),
        isSupportedOnAllAsics(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
  };
  verifyAcrossWarmBoots(setup, [] {});
}

TEST_F(AgentVoqSwitchFullScaleDsfNodesTest, systemPortScaleTestWithHyperPort) {
  auto setup = [this]() {
    utility::setupRemoteIntfAndSysPorts(
        getSw(),
        isSupportedOnAllAsics(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE));
  };
  auto verify = [this]() {
    utility::setupRemoteIntfAndSysPorts(
        getSw(),
        isSupportedOnAllAsics(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE),
        true /* useHyperPort*/);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
