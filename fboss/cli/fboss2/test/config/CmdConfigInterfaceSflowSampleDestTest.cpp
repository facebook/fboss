// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <string>

#include "fboss/cli/fboss2/commands/config/interface/sflow/sample_dest/CmdConfigInterfaceSflowSampleDest.h"
#include "fboss/cli/fboss2/commands/delete/interface/sflow/sample_dest/CmdDeleteInterfaceSflowSampleDest.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceSflowSampleDestTestFixture : public CmdConfigTestBase {
 public:
  // Production ports carry no sampleDest (unset optional); eth1/2/1 has a
  // non-zero sFlowEgressRate to exercise the mirror-destination restriction.
  CmdConfigInterfaceSflowSampleDestTestFixture()
      : CmdConfigTestBase(
            "fboss_sflow_sample_dest_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000,
        "ingressVlan": 1,
        "sFlowIngressRate": 0,
        "sFlowEgressRate": 0
      },
      {
        "logicalID": 2,
        "name": "eth1/2/1",
        "state": 2,
        "speed": 100000,
        "ingressVlan": 1,
        "sFlowIngressRate": 512,
        "sFlowEgressRate": 512
      }
    ],
    "vlans": [
      {"id": 1, "name": "vlan1", "routable": true, "intfID": 1}
    ],
    "interfaces": [
      {"intfID": 1, "vlanID": 1, "routerID": 0, "type": 1, "mtu": 9412, "name": "vlan1"}
    ]
  }
})") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession(
        "config interface sflow sample-dest eth1/1/1", "cpu");
  }

  static std::optional<cfg::SampleDestination> sampleDestOf(
      const std::string& portName) {
    auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
    for (const auto& port : *swConfig.ports()) {
      if (*port.name() == portName) {
        return port.sampleDest().to_optional();
      }
    }
    throw std::runtime_error("port not found: " + portName);
  }
};

// SampleDestValue validation

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, valueCpu) {
  EXPECT_EQ(
      SampleDestValue({"cpu"}).getDestination(), cfg::SampleDestination::CPU);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, valueMirror) {
  EXPECT_EQ(
      SampleDestValue({"mirror"}).getDestination(),
      cfg::SampleDestination::MIRROR);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, valueCaseInsensitive) {
  EXPECT_EQ(
      SampleDestValue({"CPU"}).getDestination(), cfg::SampleDestination::CPU);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, valueInvalid) {
  EXPECT_THROW(SampleDestValue({"collector"}), std::invalid_argument);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, valueWrongArity) {
  EXPECT_THROW(SampleDestValue({}), std::invalid_argument);
  EXPECT_THROW(SampleDestValue({"cpu", "mirror"}), std::invalid_argument);
}

// config queryClient

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, setCpu) {
  ASSERT_EQ(sampleDestOf("eth1/1/1"), std::nullopt);

  auto cmd = CmdConfigInterfaceSflowSampleDest();
  utils::InterfaceList interfaces({"eth1/1/1"});
  auto result =
      cmd.queryClient(localhost(), interfaces, SampleDestValue({"cpu"}));

  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("cpu"));
  EXPECT_EQ(sampleDestOf("eth1/1/1"), cfg::SampleDestination::CPU);
  // The other port is untouched.
  EXPECT_EQ(sampleDestOf("eth1/2/1"), std::nullopt);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, setMirror) {
  auto cmd = CmdConfigInterfaceSflowSampleDest();
  utils::InterfaceList interfaces({"eth1/1/1"});
  cmd.queryClient(localhost(), interfaces, SampleDestValue({"mirror"}));

  EXPECT_EQ(sampleDestOf("eth1/1/1"), cfg::SampleDestination::MIRROR);
}

TEST_F(
    CmdConfigInterfaceSflowSampleDestTestFixture,
    mirrorRefusedWithEgressSampling) {
  // eth1/2/1 has sFlowEgressRate 512; the agent rejects MIRROR with egress
  // sampling, so the CLI must refuse before touching the config.
  auto cmd = CmdConfigInterfaceSflowSampleDest();
  utils::InterfaceList interfaces({"eth1/2/1"});
  EXPECT_THROW(
      cmd.queryClient(localhost(), interfaces, SampleDestValue({"mirror"})),
      std::invalid_argument);
  EXPECT_EQ(sampleDestOf("eth1/2/1"), std::nullopt);

  // CPU is fine on the same port.
  cmd.queryClient(localhost(), interfaces, SampleDestValue({"cpu"}));
  EXPECT_EQ(sampleDestOf("eth1/2/1"), cfg::SampleDestination::CPU);
}

TEST_F(
    CmdConfigInterfaceSflowSampleDestTestFixture,
    mixedListReportsSkippedNonPortNames) {
  // vlan1 resolves only as an L3 interface — sampleDest is a Port attribute,
  // so the command must name it as skipped instead of silently succeeding.
  auto cmd = CmdConfigInterfaceSflowSampleDest();
  utils::InterfaceList interfaces({"eth1/1/1", "vlan1"});
  auto result =
      cmd.queryClient(localhost(), interfaces, SampleDestValue({"cpu"}));

  EXPECT_THAT(result, HasSubstr("skipped (no port): vlan1"));
  EXPECT_EQ(sampleDestOf("eth1/1/1"), cfg::SampleDestination::CPU);

  auto deleteCmd = CmdDeleteInterfaceSflowSampleDest();
  auto deleteResult = deleteCmd.queryClient(localhost(), interfaces);
  EXPECT_THAT(deleteResult, HasSubstr("skipped (no port): vlan1"));
  EXPECT_EQ(sampleDestOf("eth1/1/1"), std::nullopt);
}

TEST_F(
    CmdConfigInterfaceSflowSampleDestTestFixture,
    setThrowsOnEmptyInterfaceList) {
  auto cmd = CmdConfigInterfaceSflowSampleDest();
  utils::InterfaceList emptyInterfaces({});
  EXPECT_THROW(
      cmd.queryClient(localhost(), emptyInterfaces, SampleDestValue({"cpu"})),
      std::invalid_argument);
}

// delete queryClient

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, deleteClearsSampleDest) {
  auto configCmd = CmdConfigInterfaceSflowSampleDest();
  utils::InterfaceList interfaces({"eth1/1/1"});
  configCmd.queryClient(localhost(), interfaces, SampleDestValue({"cpu"}));
  ASSERT_EQ(sampleDestOf("eth1/1/1"), cfg::SampleDestination::CPU);

  auto deleteCmd = CmdDeleteInterfaceSflowSampleDest();
  auto result = deleteCmd.queryClient(localhost(), interfaces);

  EXPECT_THAT(result, HasSubstr("Reset sFlow sample destination"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_EQ(sampleDestOf("eth1/1/1"), std::nullopt);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, deleteIsIdempotent) {
  auto cmd = CmdDeleteInterfaceSflowSampleDest();
  utils::InterfaceList interfaces({"eth1/1/1"});

  cmd.queryClient(localhost(), interfaces);
  cmd.queryClient(localhost(), interfaces);

  EXPECT_EQ(sampleDestOf("eth1/1/1"), std::nullopt);
}

TEST_F(
    CmdConfigInterfaceSflowSampleDestTestFixture,
    deleteThrowsOnEmptyInterfaceList) {
  auto cmd = CmdDeleteInterfaceSflowSampleDest();
  utils::InterfaceList emptyInterfaces({});
  EXPECT_THROW(
      cmd.queryClient(localhost(), emptyInterfaces), std::invalid_argument);
}

} // namespace facebook::fboss
