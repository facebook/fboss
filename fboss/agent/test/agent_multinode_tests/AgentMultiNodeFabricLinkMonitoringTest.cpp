// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#ifndef IS_OSS

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeDsfUtils.h"
#include "fboss/agent/test/agent_multinode_tests/AgentMultiNodeTest.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

namespace {

// SystemPortOffset is added to fabric port ID to derive the SystemPortId.
// Different offsets ensure appropriate systemPortId ranges for each mode.
constexpr int kFabricLinkMonitoringSystemPortOffsetSingleStage = -480;
constexpr int kFabricLinkMonitoringSystemPortOffsetDualStage = -1015;

} // namespace

class AgentMultiNodeFabricLinkMonitoringTest : public AgentMultiNodeTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    // Load config from file instead of calling parent's initialConfig(),
    // which uses getL3Asics() that fails for fabric switches.
    XLOG(DBG0) << "initialConfig() loading config from file " << FLAGS_config;

    auto agentConfig = AgentConfig::fromFile(FLAGS_config);
    auto config = *agentConfig->thrift.sw();

    // Check if this is a VoQ switch or Fabric switch
    bool isVoqSwitch = false;
    for (const auto& [switchId, hwAsic] :
         ensemble.getSw()->getHwAsicTable()->getHwAsics()) {
      if (hwAsic->getSwitchType() == cfg::SwitchType::VOQ) {
        isVoqSwitch = true;
        break;
      }
    }

    if (isVoqSwitch) {
      // Add fabric link monitoring system port offset for VoQ switches only
      // This setting is not applicable for fabric switches (FDSW/SDSW)
      config.switchSettings()->fabricLinkMonitoringSystemPortOffset() =
          isDualStage3Q2QMode()
          ? kFabricLinkMonitoringSystemPortOffsetDualStage
          : kFabricLinkMonitoringSystemPortOffsetSingleStage;
    }

    return config;
  }

 private:
  void setCmdLineFlagOverrides() const override {
    AgentMultiNodeTest::setCmdLineFlagOverrides();
    // This is needed on all DSF roles
    FLAGS_enable_fabric_link_monitoring = true;
  }
};

} // namespace facebook::fboss

#endif // IS_OSS
