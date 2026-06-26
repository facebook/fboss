/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"

namespace facebook::fboss {
enum class MirrorType {
  INGRESS_SPAN,
  EGRESS_SPAN,
  INGRESS_ERSPAN,
  EGRESS_ERSPAN
};

template <MirrorType mirrorTypeInput>
struct MirrorScaleTestType {
  static constexpr auto mirrorType = mirrorTypeInput;
};

using MirrorIngressSpan = MirrorScaleTestType<MirrorType::INGRESS_SPAN>;
using MirrorEgressSpan = MirrorScaleTestType<MirrorType::EGRESS_SPAN>;
using MirrorIngressErspan = MirrorScaleTestType<MirrorType::INGRESS_ERSPAN>;
using MirrorEgressErspan = MirrorScaleTestType<MirrorType::EGRESS_ERSPAN>;

// The IPv4/IPv6 axis is no longer a separate test instantiation; ERSPAN tunnels
// are created with both families interleaved within a single test (see
// addMirrorConfig).
using MirrorTypes = ::testing::Types<
    MirrorIngressSpan,
    MirrorEgressSpan,
    MirrorIngressErspan,
    MirrorEgressErspan>;

template <typename MirrorT>
class AgentMirroringScaleTest : public AgentHwTest {
 public:
  // Only the ingress/egress capability is required to run. v6 ERSPAN is
  // exercised opportunistically (see addMirrorConfig), gated at runtime on
  // HwAsic::Feature::ERSPANv6, so it is not listed as a required feature here
  // (that would skip v4 ERSPAN coverage on platforms without v6 ERSPAN).
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (
        MirrorT::mirrorType == MirrorType::INGRESS_SPAN ||
        MirrorT::mirrorType == MirrorType::INGRESS_ERSPAN) {
      return {ProductionFeature::INGRESS_MIRRORING};
    } else {
      return {ProductionFeature::EGRESS_MIRRORING};
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
    if (maxMirrors * 2 > this->getAllPorts(ensemble).size()) {
      throw FbossError("Not enough ports to create mirrors");
    }
    for (int i = 0; i < maxMirrors; i++) {
      addMirrorConfig(&ensemble, &cfg, i);
    }
    return cfg;
  }

  bool isErspan() const {
    return MirrorT::mirrorType == MirrorType::INGRESS_ERSPAN ||
        MirrorT::mirrorType == MirrorType::EGRESS_ERSPAN;
  }

  bool erspanV6Supported(const AgentEnsemble* ensemble) const {
    auto asic = checkSameAndGetAsicForTesting(ensemble->getL3Asics());
    return asic->isSupported(HwAsic::Feature::ERSPANv6);
  }

  void addMirrorConfig(
      const AgentEnsemble* ensemble,
      cfg::SwitchConfig* cfg,
      int mirrorIndex) const {
    const auto maxMirrors = getMaxMirrorsEntries(ensemble->getL3Asics());
    const std::string mirrorName =
        getMirrorMode() + "-" + std::to_string(mirrorIndex);
    // Interleave v4 and v6 ERSPAN tunnels across mirror indices so both address
    // families are exercised at scale within a single test. v6 is used only
    // where the ASIC supports v6 ERSPAN; otherwise fall back to v4. SPAN
    // mirrors have no tunnel, so the family is irrelevant (use v4).
    const bool useV6 =
        isErspan() && erspanV6Supported(ensemble) && (mirrorIndex % 2 == 1);
    if (useV6) {
      utility::addMirrorConfig<folly::IPAddressV6>(
          cfg,
          *ensemble,
          mirrorName,
          false /* truncate */,
          utility::kDscpDefault,
          maxMirrors + mirrorIndex /* mirrorToPortIndex */);
    } else {
      utility::addMirrorConfig<folly::IPAddressV4>(
          cfg,
          *ensemble,
          mirrorName,
          false /* truncate */,
          utility::kDscpDefault,
          maxMirrors + mirrorIndex /* mirrorToPortIndex */);
    }
    addPortMirrorConfig(cfg, *ensemble, mirrorName, mirrorIndex);
  }

  uint32_t getMaxMirrorsEntries(const std::vector<const HwAsic*>& asics) const {
    auto asic = checkSameAndGetAsicForTesting(asics);
    uint32_t maxMirrorsEntries = asic->getMaxMirrors();
    if constexpr (
        MirrorT::mirrorType == MirrorType::INGRESS_SPAN ||
        MirrorT::mirrorType == MirrorType::EGRESS_SPAN) {
      maxMirrorsEntries = maxMirrorsEntries - 0;
    }
    if (FLAGS_hyper_port) {
      maxMirrorsEntries = std::min(maxMirrorsEntries, (uint32_t)(2));
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

  std::vector<PortID> getAllPorts(const AgentEnsemble& ensemble) const {
    if (FLAGS_hyper_port) {
      return ensemble.masterLogicalHyperPortIds();
    }
    return ensemble.masterLogicalInterfacePortIds();
  }

  PortID getTrafficPort(const AgentEnsemble& ensemble, int portIndex) const {
    CHECK_LE(portIndex, getAllPorts(ensemble).size());
    return getAllPorts(ensemble)[portIndex];
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
  this->verifyAcrossWarmBoots(verify);
}

TYPED_TEST(AgentMirroringScaleTest, ExceedMaxMirroringTest) {
  auto verify = [=, this]() {
    auto mirrors = this->getProgrammedState()->getMirrors()->numNodes();
    CHECK_EQ(
        mirrors,
        this->getMaxMirrorsEntries(this->getAgentEnsemble()->getL3Asics()));

    auto cfg = this->getAgentEnsemble()->getSw()->getConfig();

    this->addMirrorConfig(this->getAgentEnsemble(), &cfg, mirrors + 1);
    ASSERT_THROW(this->applyNewConfig(cfg), FbossError);
  };
  verify();
}

} // namespace facebook::fboss
