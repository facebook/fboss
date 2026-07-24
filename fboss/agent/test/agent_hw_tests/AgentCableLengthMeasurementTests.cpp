// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/LinkStateToggler.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss {

namespace {
// Cable Length Measurement emits a PTP peer-delay (pDelay) request when the
// port comes up. On this platform the pDelay is a PTP-over-UDP/IPv4 frame to
// the peer-delay multicast group 224.0.0.107, UDP event port 319. Match on that
// destination IP to count it (the frame is trapped to the CPU, so it traverses
// the ingress ACL pipeline).
constexpr auto kPtpPeerDelayDstIp = "224.0.0.107/32";
constexpr auto kPtpEventUdpPort = 319;
constexpr auto kPtpAclName = "ptp-pdelay-acl";
constexpr auto kPtpAclCounterName = "ptp-pdelay-acl-stats";
constexpr auto kPtpAclTableName = "ptp-pdelay-acl-table";

// TH5 (and other XGS SDKs before the switch-level CLM attribute exists) enable
// cable length measurement via the `clm_enable` soc property. Inject it into
// the bcm_device global section of the ASIC YAML config.
void addClmEnableYamlConfig(std::string& yamlCfg) {
  const std::string kGlobalSection = "    global:\n";
  auto pos = yamlCfg.find(kGlobalSection);
  if (pos != std::string::npos) {
    yamlCfg.insert(pos + kGlobalSection.size(), "      clm_enable: 1\n");
  }
}
} // namespace

class AgentCableLengthMeasurementTest : public AgentHwTest {
 protected:
  // TH5 enables CLM via the `clm_enable` soc property (injected into the ASIC
  // YAML config below); TH6 uses the switch-level SAI attribute. Everything
  // else (per-port config, ACL) is common.
  void applyPlatformConfigOverrides(
      const cfg::SwitchConfig& sw,
      cfg::PlatformConfig& config) const override {
    if (checkSameAndGetAsicType(sw) == cfg::AsicType::ASIC_TYPE_TOMAHAWK5) {
      utility::modifyPlatformConfig(
          config,
          [](std::string& yamlCfg) { addClmEnableYamlConfig(yamlCfg); },
          [](std::map<std::string, std::string>& cfg) {
            cfg["clm_enable"] = "1";
          });
    }
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);

    // Switch-level CLM enable. TH6 uses the switch-level SAI attribute
    // (SAI_SWITCH_ATTR_CABLE_PROPAGATION_DELAY_MEASUREMENT). TH5 uses the
    // `clm_enable` soc property instead (see applyPlatformConfigOverrides), so
    // don't set the switch attribute there.
    if (checkSameAndGetAsicType(cfg) != cfg::AsicType::ASIC_TYPE_TOMAHAWK5) {
      cfg.switchSettings()->measureCableLengths() = true;
    }

    // Enable CLM on the port under test (per-port SAI_PORT_ATTR_* wiring).
    auto testPort = getTestPortId(ensemble);
    for (auto& portCfg : *cfg.ports()) {
      if (PortID(*portCfg.logicalID()) == testPort) {
        portCfg.clmEnable() = true;
      }
    }

    addPtpCountingAcl(cfg);
    return cfg;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::CABLE_LENGTH_MEASUREMENT};
  }

  PortID getTestPortId(const AgentEnsemble& ensemble) const {
    return ensemble.masterLogicalInterfacePortIds()[0];
  }

  // Install an ingress ACL that matches PTP pDelay frames by destination IP +
  // UDP port and counts packets. In the AgentHwTest single-box setup ports are
  // brought up in loopback, so the pDelay frame the SDK emits loops back into
  // the same port, is trapped to the CPU, and hits this ingress ACL.
  void addPtpCountingAcl(cfg::SwitchConfig& cfg) const {
    cfg::AclEntry acl;
    acl.name() = kPtpAclName;
    acl.actionType() = cfg::AclActionType::PERMIT;
    acl.dstIp() = kPtpPeerDelayDstIp;
    acl.l4DstPort() = kPtpEventUdpPort;

    if (FLAGS_enable_acl_table_group) {
      utility::addAclTable(
          &cfg,
          kPtpAclTableName,
          1 /* priority */,
          {cfg::AclTableActionType::PACKET_ACTION,
           cfg::AclTableActionType::COUNTER},
          {cfg::AclTableQualifier::DST_IPV4,
           cfg::AclTableQualifier::L4_DST_PORT});
      utility::addAclEntry(&cfg, acl, kPtpAclTableName, cfg::AclStage::INGRESS);
    } else {
      utility::addAcl(&cfg, acl, cfg::AclStage::INGRESS);
    }
    utility::addAclStat(
        &cfg, kPtpAclName, kPtpAclCounterName, {cfg::CounterType::PACKETS});
  }
};

// With cable length measurement enabled the SDK emits PTP pDelay requests. Flap
// the port to drive a port-up and verify the ACL matching the pDelay
// (dstIp 224.0.0.107, UDP port 319) counts the emitted packets.
TEST_F(AgentCableLengthMeasurementTest, PtpSentWithClmEnabled) {
  auto setup = [this]() { applyNewConfig(initialConfig(*getAgentEnsemble())); };

  auto verify = [this]() {
    auto ensemble = getAgentEnsemble();
    auto testPort = getTestPortId(*ensemble);

    auto aclPktsBefore =
        utility::getAclInOutPackets(getSw(), kPtpAclCounterName);

    ensemble->getLinkToggler()->bringDownPorts({testPort});
    ensemble->getLinkToggler()->bringUpPorts({testPort});

    WITH_RETRIES({
      auto aclPktsAfter =
          utility::getAclInOutPackets(getSw(), kPtpAclCounterName);
      XLOG(DBG2) << "PTP pDelay ACL pkts " << aclPktsBefore << " -> "
                 << aclPktsAfter;
      EXPECT_EVENTUALLY_GT(aclPktsAfter, aclPktsBefore);
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
