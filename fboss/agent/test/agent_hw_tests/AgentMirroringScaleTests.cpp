/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"

#include <folly/IPAddress.h>

namespace facebook::fboss {
enum class MirrorType {
  INGRESS_SPAN,
  EGRESS_SPAN,
  INGRESS_ERSPAN,
  EGRESS_ERSPAN
};

template <typename AddrType, MirrorType mirrorTypeInput>
struct MirrorScaleTestType {
  using AddrT = AddrType;
  static constexpr auto mirrorType = mirrorTypeInput;
};

using MirrorIngressSpanV4 =
    MirrorScaleTestType<folly::IPAddressV4, MirrorType::INGRESS_SPAN>;
using MirrorEgressSpanV4 =
    MirrorScaleTestType<folly::IPAddressV4, MirrorType::EGRESS_SPAN>;
using MirrorIngressErspanV4 =
    MirrorScaleTestType<folly::IPAddressV4, MirrorType::INGRESS_ERSPAN>;
using MirrorEgressErspanV4 =
    MirrorScaleTestType<folly::IPAddressV4, MirrorType::EGRESS_ERSPAN>;
using MirrorIngressSpanV6 =
    MirrorScaleTestType<folly::IPAddressV6, MirrorType::INGRESS_SPAN>;
using MirrorEgressSpanV6 =
    MirrorScaleTestType<folly::IPAddressV6, MirrorType::EGRESS_SPAN>;
using MirrorIngressErspanV6 =
    MirrorScaleTestType<folly::IPAddressV6, MirrorType::INGRESS_ERSPAN>;
using MirrorEgressErspanV6 =
    MirrorScaleTestType<folly::IPAddressV6, MirrorType::EGRESS_ERSPAN>;

using MirrorTypes = ::testing::Types<
    MirrorIngressSpanV4,
    MirrorEgressSpanV4,
    MirrorIngressErspanV4,
    MirrorEgressErspanV4,
    MirrorIngressSpanV6,
    MirrorEgressSpanV6,
    MirrorIngressErspanV6,
    MirrorEgressErspanV6>;

template <typename MirrorT>
class AgentMirroringScaleTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if constexpr (std::is_same<typename MirrorT::AddrT, folly::IPAddressV6>::
                      value) {
      if constexpr (MirrorT::mirrorType == MirrorType::INGRESS_ERSPAN) {
        return {
            production_features::ProductionFeature::ERSPANV6_MIRRORING,
            production_features::ProductionFeature::INGRESS_MIRRORING};
      }
      if constexpr (MirrorT::mirrorType == MirrorType::EGRESS_ERSPAN) {
        return {
            production_features::ProductionFeature::ERSPANV6_MIRRORING,
            production_features::ProductionFeature::EGRESS_MIRRORING};
      }
    }
    if constexpr (
        MirrorT::mirrorType == MirrorType::INGRESS_SPAN ||
        MirrorT::mirrorType == MirrorType::INGRESS_ERSPAN) {
      return {production_features::ProductionFeature::INGRESS_MIRRORING};
    } else if constexpr (
        MirrorT::mirrorType == MirrorType::EGRESS_SPAN ||
        MirrorT::mirrorType == MirrorType::EGRESS_ERSPAN) {
      return {production_features::ProductionFeature::EGRESS_MIRRORING};
    }
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    const auto maxMirrors = getMaxMirrorsEntries(ensemble.getL3Asics());
    XLOG(DBG2) << " Creating " << maxMirrors << " "
               << (MirrorT::mirrorType == MirrorType::INGRESS_SPAN ||
                           MirrorT::mirrorType == MirrorType::INGRESS_ERSPAN
                       ? "ingress"
                       : "egress")
               << " mirrors";
    // single mirror session need pair of ports to work
    if (maxMirrors * 2 > ensemble.masterLogicalInterfacePortIds().size()) {
      throw FbossError("Not enough ports to create mirrors");
    }
    for (int i = 0; i < maxMirrors; i++) {
      const std::string mirrorName = getMirrorMode() + "-" + std::to_string(i);
      utility::addMirrorConfig<typename MirrorT::AddrT>(
          &cfg,
          ensemble,
          mirrorName,
          false /* truncate */,
          utility::kDscpDefault,
          maxMirrors + i /* mirrorToPortIndex */);
      addPortMirrorConfig(&cfg, ensemble, mirrorName, i);
    }
    return cfg;
  }

  uint32_t getMaxMirrorsEntries(const std::vector<const HwAsic*>& asics) const {
    auto asic = utility::checkSameAndGetAsic(asics);
    uint32_t maxMirrorsEntries = asic->getMaxMirrors();
    if constexpr (
        MirrorT::mirrorType == MirrorType::INGRESS_SPAN ||
        MirrorT::mirrorType == MirrorType::EGRESS_SPAN) {
      maxMirrorsEntries = maxMirrorsEntries - 0;
    }
    CHECK_GT(maxMirrorsEntries, 0);
    return maxMirrorsEntries;
  }

 protected:
  std::string getMirrorMode() const {
    if constexpr (MirrorT::mirrorType == MirrorType::INGRESS_SPAN) {
      return utility::kIngressSpan;
    }
    if constexpr (MirrorT::mirrorType == MirrorType::EGRESS_SPAN) {
      return utility::kEgressSpan;
    }
    if constexpr (MirrorT::mirrorType == MirrorType::INGRESS_ERSPAN) {
      return utility::kIngressErspan;
    }
    if constexpr (MirrorT::mirrorType == MirrorType::EGRESS_ERSPAN) {
      return utility::kEgressErspan;
    }
    throw FbossError("Invalid mirror type");
  }

  PortID getTrafficPort(const AgentEnsemble& ensemble, int portIndex) const {
    CHECK_LE(portIndex, ensemble.masterLogicalInterfacePortIds().size());
    return ensemble.masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[portIndex];
  }
  void addPortMirrorConfig(
      cfg::SwitchConfig* cfg,
      const AgentEnsemble& ensemble,
      const std::string& mirrorName,
      const int portIndex) const {
    auto trafficPort = getTrafficPort(ensemble, portIndex);
    auto portConfig = utility::findCfgPort(*cfg, trafficPort);
    if constexpr (
        MirrorT::mirrorType == MirrorType::INGRESS_SPAN ||
        MirrorT::mirrorType == MirrorType::INGRESS_ERSPAN) {
      XLOG(DBG2) << "ingressMirror: " << mirrorName;
      portConfig->ingressMirror() = mirrorName;
    } else if constexpr (
        MirrorT::mirrorType == MirrorType::EGRESS_SPAN ||
        MirrorT::mirrorType == MirrorType::EGRESS_ERSPAN) {
      XLOG(DBG2) << "egressMirror: " << mirrorName;
      portConfig->egressMirror() = mirrorName;
    } else {
      throw FbossError("Invalid mirror name ", mirrorName);
    }
  }
};

TYPED_TEST_SUITE(AgentMirroringScaleTest, MirrorTypes);
TYPED_TEST(AgentMirroringScaleTest, MaxMirroringTest) {
  auto verify = [=, this]() {
    auto mirrors = this->getProgrammedState()->getMirrors()->numNodes();
    CHECK_EQ(
        mirrors,
        this->getMaxMirrorsEntries(this->getAgentEnsemble()->getL3Asics()));
  };
  this->verifyAcrossWarmBoots([] {}, verify);
}
} // namespace facebook::fboss
