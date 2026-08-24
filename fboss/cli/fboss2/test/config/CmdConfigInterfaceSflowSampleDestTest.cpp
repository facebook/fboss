// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <string>

#include "fboss/cli/fboss2/commands/config/interface/sflow/CmdConfigInterfaceSflow.h"
#include "fboss/cli/fboss2/commands/delete/interface/sflow/CmdDeleteInterfaceSflow.h"
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

  static int64_t ingressRateOf(const std::string& portName) {
    auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
    for (const auto& port : *swConfig.ports()) {
      if (*port.name() == portName) {
        return *port.sFlowIngressRate();
      }
    }
    throw std::runtime_error("port not found: " + portName);
  }

  static int64_t egressRateOf(const std::string& portName) {
    auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
    for (const auto& port : *swConfig.ports()) {
      if (*port.name() == portName) {
        return *port.sFlowEgressRate();
      }
    }
    throw std::runtime_error("port not found: " + portName);
  }
};

// SflowAttrArgs / SflowDeleteAttrArg arity validation

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, configArgsWrongArity) {
  EXPECT_THROW(SflowAttrArgs({}), std::invalid_argument);
  EXPECT_THROW(SflowAttrArgs({"sample-dest"}), std::invalid_argument);
  EXPECT_THROW(
      SflowAttrArgs({"sample-dest", "cpu", "extra"}), std::invalid_argument);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, deleteArgsWrongArity) {
  EXPECT_THROW(SflowDeleteAttrArg({}), std::invalid_argument);
  EXPECT_THROW(
      SflowDeleteAttrArg({"sample-dest", "extra"}), std::invalid_argument);
}

// config queryClient

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, setCpu) {
  ASSERT_EQ(sampleDestOf("eth1/1/1"), std::nullopt);

  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  auto result = cmd.queryClient(
      localhost(), interfaces, SflowAttrArgs({"sample-dest", "cpu"}));

  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("cpu"));
  EXPECT_EQ(sampleDestOf("eth1/1/1"), cfg::SampleDestination::CPU);
  // The other port is untouched.
  EXPECT_EQ(sampleDestOf("eth1/2/1"), std::nullopt);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, setMirror) {
  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  cmd.queryClient(
      localhost(), interfaces, SflowAttrArgs({"sample-dest", "mirror"}));

  EXPECT_EQ(sampleDestOf("eth1/1/1"), cfg::SampleDestination::MIRROR);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, valueCaseInsensitive) {
  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  cmd.queryClient(
      localhost(), interfaces, SflowAttrArgs({"SAMPLE-DEST", "CPU"}));

  EXPECT_EQ(sampleDestOf("eth1/1/1"), cfg::SampleDestination::CPU);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, valueInvalid) {
  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  EXPECT_THROW(
      cmd.queryClient(
          localhost(), interfaces, SflowAttrArgs({"sample-dest", "collector"})),
      std::invalid_argument);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, unknownAttr) {
  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  EXPECT_THROW(
      cmd.queryClient(
          localhost(), interfaces, SflowAttrArgs({"bogus-attr", "100"})),
      std::invalid_argument);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, setIngressRate) {
  ASSERT_EQ(ingressRateOf("eth1/1/1"), 0);

  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  auto result = cmd.queryClient(
      localhost(), interfaces, SflowAttrArgs({"ingress-rate", "256"}));

  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("256"));
  EXPECT_EQ(ingressRateOf("eth1/1/1"), 256);
  // The other port is untouched.
  EXPECT_EQ(ingressRateOf("eth1/2/1"), 512);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, setEgressRate) {
  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  auto result = cmd.queryClient(
      localhost(), interfaces, SflowAttrArgs({"egress-rate", "256"}));

  EXPECT_THAT(result, HasSubstr("256"));
  EXPECT_EQ(egressRateOf("eth1/1/1"), 256);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, rateValueInvalid) {
  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  EXPECT_THROW(
      cmd.queryClient(
          localhost(), interfaces, SflowAttrArgs({"ingress-rate", "abc"})),
      std::invalid_argument);
  EXPECT_THROW(
      cmd.queryClient(
          localhost(), interfaces, SflowAttrArgs({"egress-rate", "-5"})),
      std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSflowSampleDestTestFixture,
    egressRateRefusedWhenMirror) {
  // eth1/1/1 starts with sFlowEgressRate 0, so mirror is accepted first.
  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  cmd.queryClient(
      localhost(), interfaces, SflowAttrArgs({"sample-dest", "mirror"}));
  ASSERT_EQ(sampleDestOf("eth1/1/1"), cfg::SampleDestination::MIRROR);

  // A non-zero egress-rate now conflicts with the existing mirror
  // destination, same constraint as setting mirror onto a non-zero rate.
  EXPECT_THROW(
      cmd.queryClient(
          localhost(), interfaces, SflowAttrArgs({"egress-rate", "100"})),
      std::invalid_argument);
  EXPECT_EQ(egressRateOf("eth1/1/1"), 0);

  // Zero is always fine.
  cmd.queryClient(localhost(), interfaces, SflowAttrArgs({"egress-rate", "0"}));
  EXPECT_EQ(egressRateOf("eth1/1/1"), 0);
}

TEST_F(
    CmdConfigInterfaceSflowSampleDestTestFixture,
    mirrorRefusedWithEgressSampling) {
  // eth1/2/1 has sFlowEgressRate 512; the agent rejects MIRROR with egress
  // sampling, so the CLI must refuse before touching the config.
  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/2/1"});
  EXPECT_THROW(
      cmd.queryClient(
          localhost(), interfaces, SflowAttrArgs({"sample-dest", "mirror"})),
      std::invalid_argument);
  EXPECT_EQ(sampleDestOf("eth1/2/1"), std::nullopt);

  // CPU is fine on the same port.
  cmd.queryClient(
      localhost(), interfaces, SflowAttrArgs({"sample-dest", "cpu"}));
  EXPECT_EQ(sampleDestOf("eth1/2/1"), cfg::SampleDestination::CPU);
}

TEST_F(
    CmdConfigInterfaceSflowSampleDestTestFixture,
    mixedListReportsSkippedNonPortNames) {
  // vlan1 resolves only as an L3 interface — sampleDest is a Port attribute,
  // so the command must name it as skipped instead of silently succeeding.
  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1", "vlan1"});
  auto result = cmd.queryClient(
      localhost(), interfaces, SflowAttrArgs({"sample-dest", "cpu"}));

  EXPECT_THAT(result, HasSubstr("skipped (no port): vlan1"));
  EXPECT_EQ(sampleDestOf("eth1/1/1"), cfg::SampleDestination::CPU);

  auto deleteCmd = CmdDeleteInterfaceSflow();
  auto deleteResult = deleteCmd.queryClient(
      localhost(), interfaces, SflowDeleteAttrArg({"sample-dest"}));
  EXPECT_THAT(deleteResult, HasSubstr("skipped (no port): vlan1"));
  EXPECT_EQ(sampleDestOf("eth1/1/1"), std::nullopt);
}

TEST_F(
    CmdConfigInterfaceSflowSampleDestTestFixture,
    setThrowsOnEmptyInterfaceList) {
  auto cmd = CmdConfigInterfaceSflow();
  utils::InterfaceList emptyInterfaces({});
  EXPECT_THROW(
      cmd.queryClient(
          localhost(), emptyInterfaces, SflowAttrArgs({"sample-dest", "cpu"})),
      std::invalid_argument);
}

// delete queryClient

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, deleteClearsSampleDest) {
  auto configCmd = CmdConfigInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  configCmd.queryClient(
      localhost(), interfaces, SflowAttrArgs({"sample-dest", "cpu"}));
  ASSERT_EQ(sampleDestOf("eth1/1/1"), cfg::SampleDestination::CPU);

  auto deleteCmd = CmdDeleteInterfaceSflow();
  auto result = deleteCmd.queryClient(
      localhost(), interfaces, SflowDeleteAttrArg({"sample-dest"}));

  EXPECT_THAT(result, HasSubstr("Reset sFlow sample destination"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_EQ(sampleDestOf("eth1/1/1"), std::nullopt);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, deleteIsIdempotent) {
  auto cmd = CmdDeleteInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});

  cmd.queryClient(localhost(), interfaces, SflowDeleteAttrArg({"sample-dest"}));
  cmd.queryClient(localhost(), interfaces, SflowDeleteAttrArg({"sample-dest"}));

  EXPECT_EQ(sampleDestOf("eth1/1/1"), std::nullopt);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, deleteUnknownAttrThrows) {
  auto cmd = CmdDeleteInterfaceSflow();
  utils::InterfaceList interfaces({"eth1/1/1"});
  EXPECT_THROW(
      cmd.queryClient(
          localhost(), interfaces, SflowDeleteAttrArg({"bogus-attr"})),
      std::invalid_argument);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, deleteClearsIngressRate) {
  ASSERT_EQ(ingressRateOf("eth1/2/1"), 512);
  utils::InterfaceList interfaces({"eth1/2/1"});

  auto deleteCmd = CmdDeleteInterfaceSflow();
  auto result = deleteCmd.queryClient(
      localhost(), interfaces, SflowDeleteAttrArg({"ingress-rate"}));

  EXPECT_THAT(result, HasSubstr("Reset sFlow ingress-rate"));
  EXPECT_EQ(ingressRateOf("eth1/2/1"), 0);
  // egress-rate is untouched.
  EXPECT_EQ(egressRateOf("eth1/2/1"), 512);
}

TEST_F(CmdConfigInterfaceSflowSampleDestTestFixture, deleteClearsEgressRate) {
  ASSERT_EQ(egressRateOf("eth1/2/1"), 512);
  utils::InterfaceList interfaces({"eth1/2/1"});

  auto deleteCmd = CmdDeleteInterfaceSflow();
  auto result = deleteCmd.queryClient(
      localhost(), interfaces, SflowDeleteAttrArg({"egress-rate"}));

  EXPECT_THAT(result, HasSubstr("Reset sFlow egress-rate"));
  EXPECT_EQ(egressRateOf("eth1/2/1"), 0);
}

TEST_F(
    CmdConfigInterfaceSflowSampleDestTestFixture,
    deleteThrowsOnEmptyInterfaceList) {
  auto cmd = CmdDeleteInterfaceSflow();
  utils::InterfaceList emptyInterfaces({});
  EXPECT_THROW(
      cmd.queryClient(
          localhost(), emptyInterfaces, SflowDeleteAttrArg({"sample-dest"})),
      std::invalid_argument);
}

} // namespace facebook::fboss
