// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"
#include "fboss/agent/platforms/tests/utils/TestPlatformTypes.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

namespace {

cfg::PortSpeed getPortSpeed(FlexPortMode flexMode) {
  switch (flexMode) {
    case FlexPortMode::FOURX10G:
      return cfg::PortSpeed::XG;
    case FlexPortMode::FOURX25G:
      return cfg::PortSpeed::TWENTYFIVEG;
    case FlexPortMode::ONEX40G:
      return cfg::PortSpeed::FORTYG;
    case FlexPortMode::TWOX50G:
      return cfg::PortSpeed::FIFTYG;
    case FlexPortMode::ONEX100G:
      return cfg::PortSpeed::HUNDREDG;
    case FlexPortMode::ONEX400G:
      return cfg::PortSpeed::FOURHUNDREDG;
  }
  throw FbossError("invalid FlexConfig Mode");
}

cfg::PortSpeed getDisabledLaneSpeed(FlexPortMode flexMode) {
  switch (flexMode) {
    case FlexPortMode::FOURX10G:
      return cfg::PortSpeed::XG;
    case FlexPortMode::FOURX25G:
      return cfg::PortSpeed::TWENTYFIVEG;
    case FlexPortMode::ONEX40G:
      return cfg::PortSpeed::XG;
    case FlexPortMode::TWOX50G:
      return cfg::PortSpeed::TWENTYFIVEG;
    case FlexPortMode::ONEX100G:
      return cfg::PortSpeed::TWENTYFIVEG;
    case FlexPortMode::ONEX400G:
      return cfg::PortSpeed::HUNDREDG;
  }
  throw FbossError("invalid FlexConfig Mode");
}

bool portGroupSupportsSpeed(
    const PlatformMapping* platformMapping,
    const std::vector<PortID>& allPortsInGroup,
    cfg::PortSpeed speed) {
  const auto& platformPorts = platformMapping->getPlatformPorts();
  for (auto portID : allPortsInGroup) {
    auto it = platformPorts.find(static_cast<int32_t>(portID));
    if (it == platformPorts.end()) {
      continue;
    }
    for (const auto& profile : *it->second.supportedProfiles()) {
      auto profileCfg = platformMapping->getPortProfileConfig(
          PlatformPortProfileConfigMatcher(profile.first, portID));
      if (profileCfg.has_value() && *profileCfg->speed() == speed) {
        return true;
      }
    }
  }
  return false;
}

int getNumEnabledPorts(FlexPortMode flexMode) {
  switch (flexMode) {
    case FlexPortMode::FOURX10G:
    case FlexPortMode::FOURX25G:
      return 4;
    case FlexPortMode::TWOX50G:
      return 2;
    case FlexPortMode::ONEX40G:
    case FlexPortMode::ONEX100G:
    case FlexPortMode::ONEX400G:
      return 1;
  }
  throw FbossError("invalid FlexConfig Mode");
}

void applyFlexConfig(
    cfg::SwitchConfig* config,
    FlexPortMode flexMode,
    const std::vector<PortID>& allPortsInGroup,
    const PlatformMapping* platformMapping,
    bool supportsAddRemovePort) {
  auto enabledSpeed = getPortSpeed(flexMode);
  auto disabledSpeed = getDisabledLaneSpeed(flexMode);
  int numEnabled = getNumEnabledPorts(flexMode);
  const auto& platformPorts = platformMapping->getPlatformPorts();

  for (auto portIdx = 0; portIdx < allPortsInGroup.size(); portIdx++) {
    auto portID = allPortsInGroup.at(portIdx);
    bool isEnabled = false;
    if (numEnabled == 4) {
      isEnabled = true;
    } else if (numEnabled == 2) {
      isEnabled = (portIdx % 2 == 0);
    } else {
      isEnabled = (portIdx == 0);
    }

    auto portCfgIt = std::find_if(
        config->ports()->begin(), config->ports()->end(), [portID](auto& port) {
          return static_cast<PortID>(*port.logicalID()) == portID;
        });

    if (portCfgIt == config->ports()->end()) {
      if (isEnabled || !supportsAddRemovePort) {
        throw FbossError("Port:", portID, " doesn't exist in the config");
      }
      continue;
    }

    if (!isEnabled && supportsAddRemovePort) {
      config->ports()->erase(portCfgIt);
      auto vlanMemberPort = std::find_if(
          config->vlanPorts()->begin(),
          config->vlanPorts()->end(),
          [portID](auto& vlanPort) {
            return static_cast<PortID>(*vlanPort.logicalPort()) == portID;
          });
      if (vlanMemberPort != config->vlanPorts()->end()) {
        config->vlanPorts()->erase(vlanMemberPort);
      }
      continue;
    }

    portCfgIt->state() =
        isEnabled ? cfg::PortState::ENABLED : cfg::PortState::DISABLED;
    auto portSpeed = isEnabled ? enabledSpeed : disabledSpeed;
    if (isEnabled || supportsAddRemovePort) {
      portCfgIt->speed() = portSpeed;
    } else {
      portSpeed = *portCfgIt->speed();
    }

    auto portIt = platformPorts.find(static_cast<int32_t>(portID));
    if (portIt != platformPorts.end()) {
      for (const auto& profile : *portIt->second.supportedProfiles()) {
        auto profileCfg = platformMapping->getPortProfileConfig(
            PlatformPortProfileConfigMatcher(profile.first, portID));
        if (profileCfg.has_value() && *profileCfg->speed() == portSpeed) {
          portCfgIt->profileID() = profile.first;
          break;
        }
      }
      portCfgIt->name() = *portIt->second.mapping()->name();
    }
  }
}

} // unnamed namespace

class AgentFlexPortTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalInterfaceOrHyperPortIds(),
        true /*interfaceHasSubnet*/);
    // On BCM XGS platforms (TH3/TH4/TH5/TH6), each port macro (PM) has
    // two VCO oscillator slots. The default config may assign different
    // speeds to ports sharing a PM (e.g. 100G=VCO11, 200G=VCO12), filling
    // both slots and blocking flex port speed changes that need a different
    // VCO. Force all interface ports to 100G so PM ports share VCO11,
    // leaving a slot free.
    auto asic = ensemble.getL3Asics().front();
    if (asic->getAsicVendor() == HwAsic::AsicVendor::ASIC_VENDOR_BCM &&
        asic->getSwitchType() == cfg::SwitchType::NPU) {
      auto platformMapping = ensemble.getPlatformMapping();
      auto supportsAddRemovePort =
          ensemble.getSw()->getPlatformSupportsAddRemovePort();
      std::vector<PortID> interfacePorts;
      for (const auto& port : *cfg.ports()) {
        if (*port.portType() == cfg::PortType::INTERFACE_PORT) {
          interfacePorts.push_back(PortID(*port.logicalID()));
        }
      }
      for (auto portId : interfacePorts) {
        try {
          utility::updatePortSpeed(
              platformMapping,
              supportsAddRemovePort,
              cfg,
              portId,
              cfg::PortSpeed::HUNDREDG);
        } catch (const std::exception&) {
        }
      }
    }
    return cfg;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_FORWARDING};
  }

  void flexPortApplyConfigTest(
      FlexPortMode flexMode,
      const std::string& configName) {
    if (getAgentEnsemble()->getL3Asics().front()->getAsicVendor() !=
        HwAsic::AsicVendor::ASIC_VENDOR_BCM) {
      XLOG(DBG2) << "Flex port tests only supported on BCM platforms, skipping";
      return;
    }
    auto platformMapping = getAgentEnsemble()->getPlatformMapping();
    auto ports = masterLogicalInterfaceOrHyperPortIds();
    std::vector<PortID> allPortsinGroup;
    int index = 0;

    for (; index < ports.size(); index++) {
      allPortsinGroup =
          utility::getAllPortsInGroup(platformMapping, ports[index]);
      if (portGroupSupportsSpeed(
              platformMapping, allPortsinGroup, getPortSpeed(flexMode))) {
        break;
      }
    }

    if (index == ports.size()) {
      XLOG(DBG2) << "No ports found with matching profile for flexport mode: "
                 << configName << ", skipping";
      return;
    }

    const PortID& masterLogicalPortId = ports[index];

    auto setup = [=, this]() {
      auto asic = getAgentEnsemble()->getL3Asics().front();
      auto pm = getAgentEnsemble()->getPlatformMapping();
      auto supportsAddRemove = getAgentEnsemble()->supportsAddRemovePort();
      cfg::SwitchConfig cfg;
      if (asic->getAsicVendor() == HwAsic::AsicVendor::ASIC_VENDOR_BCM &&
          asic->getSwitchType() == cfg::SwitchType::NPU) {
        cfg = utility::oneL3IntfNPortConfig(
            pm, asic, allPortsinGroup, supportsAddRemove);
        std::vector<PortID> portsToUpdate;
        for (const auto& port : *cfg.ports()) {
          auto portId = PortID(*port.logicalID());
          if (std::find(
                  allPortsinGroup.begin(), allPortsinGroup.end(), portId) ==
              allPortsinGroup.end()) {
            portsToUpdate.push_back(portId);
          }
        }
        for (auto portId : portsToUpdate) {
          try {
            utility::updatePortSpeed(
                pm, supportsAddRemove, cfg, portId, cfg::PortSpeed::HUNDREDG);
          } catch (const std::exception&) {
          }
        }
      } else {
        cfg = this->initialConfig(*this->getAgentEnsemble());
      }
      applyFlexConfig(&cfg, flexMode, allPortsinGroup, pm, supportsAddRemove);
      applyNewConfig(cfg);
    };

    auto verify = [=, this]() {
      WITH_RETRIES({
        auto state = getProgrammedState();
        auto port = state->getPorts()->getNodeIf(masterLogicalPortId);
        ASSERT_EVENTUALLY_NE(port, nullptr);
        EXPECT_EVENTUALLY_EQ(port->getAdminState(), cfg::PortState::ENABLED);
        EXPECT_EVENTUALLY_EQ(port->getSpeed(), getPortSpeed(flexMode));
      });
    };

    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(AgentFlexPortTest, FlexPortTWOX50G) {
  flexPortApplyConfigTest(FlexPortMode::TWOX50G, "TWOX50G");
}

TEST_F(AgentFlexPortTest, FlexPortFOURX10G) {
  flexPortApplyConfigTest(FlexPortMode::FOURX10G, "FOURX10G");
}

TEST_F(AgentFlexPortTest, FlexPortFOURX25G) {
  flexPortApplyConfigTest(FlexPortMode::FOURX25G, "FOURX25G");
}

TEST_F(AgentFlexPortTest, FlexPortONEX40G) {
  flexPortApplyConfigTest(FlexPortMode::ONEX40G, "ONEX40G");
}

TEST_F(AgentFlexPortTest, FlexPortONEX100G) {
  flexPortApplyConfigTest(FlexPortMode::ONEX100G, "ONEX100G");
}

} // namespace facebook::fboss
