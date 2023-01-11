/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/LoadBalancerMap.h"

#include <memory>

using namespace facebook::fboss;

namespace {
// udf is applied on srcmod/srcport fields
constexpr int kUdfHashFields = BCM_HASH_FIELD_SRCMOD | BCM_HASH_FIELD_SRCPORT;
} // unnamed namespace

namespace facebook::fboss {

class BcmRtag7Test : public BcmTest {
 public:
  void updateLoadBalancers(std::shared_ptr<LoadBalancerMap> loadBalancerMap) {
    auto newState = getProgrammedState()->clone();
    newState->resetLoadBalancers(loadBalancerMap);
    applyNewState(newState);
  }

 protected:
  std::shared_ptr<SwitchState> setupFullEcmpHash() {
    auto noMplsFields = LoadBalancer::MPLSFields{};
    return setupHash(
        LoadBalancerID::ECMP,
        kV4Fields_,
        kV6FieldsNoFlowLabel_,
        kTransportFields_,
        std::move(noMplsFields),
        kNoUdfGroupIds_);
  }

  std::shared_ptr<SwitchState> setupHalfTrunkHash() {
    auto noTransportFields = LoadBalancer::TransportFields{};
    auto noMplsFields = LoadBalancer::MPLSFields{};
    return setupHash(
        LoadBalancerID::AGGREGATE_PORT,
        kV4Fields_,
        kV6FieldsNoFlowLabel_,
        std::move(noTransportFields),
        std::move(noMplsFields),
        kNoUdfGroupIds_);
  }

  std::shared_ptr<SwitchState> setupHashWithAllFields(LoadBalancerID id) {
    return setupHash(
        id,
        kV4Fields_,
        kV6Fields_,
        kTransportFields_,
        kMplsFields_,
        kNoUdfGroupIds_);
  }

  std::shared_ptr<SwitchState> setupFullEcmpUdfHash() {
    auto noMplsFields = LoadBalancer::MPLSFields{};
    return setupHashWithUdf(
        LoadBalancerID::ECMP,
        kV4Fields_,
        kV6FieldsNoFlowLabel_,
        kTransportFields_,
        std::move(noMplsFields),
        kUdfGroupIds_);
  }

  std::shared_ptr<SwitchState> setupHashWithUdf(
      LoadBalancerID id,
      const LoadBalancer::IPv4Fields& v4Fields,
      const LoadBalancer::IPv6Fields& v6Fields,
      const LoadBalancer::TransportFields& transportFields,
      const LoadBalancer::MPLSFields& mplsFields,
      const LoadBalancer::UdfGroupIds& udfGroupIds) {
    auto loadBalancer = makeLoadBalancer(
        id, v4Fields, v6Fields, transportFields, mplsFields, udfGroupIds);

    auto loadBalancers = getProgrammedState()->getLoadBalancers()->clone();
    if (!loadBalancers->getNodeIf(id)) {
      loadBalancers->addNode(std::move(loadBalancer));
    } else {
      loadBalancers->updateNode(std::move(loadBalancer));
    }

    auto udfConfigState = std::make_shared<UdfConfig>();
    auto udfConfig = utility::addUdfConfig();
    udfConfigState->fromThrift(udfConfig);

    auto state = getProgrammedState();
    state->modify(&state);
    state->resetUdfConfig(udfConfigState);
    state->resetLoadBalancers(std::move(loadBalancers));
    return state;
  }

  std::shared_ptr<SwitchState> setupHash(
      LoadBalancerID id,
      const LoadBalancer::IPv4Fields& v4Fields,
      const LoadBalancer::IPv6Fields& v6Fields,
      const LoadBalancer::TransportFields& transportFields,
      const LoadBalancer::MPLSFields& mplsFields,
      const LoadBalancer::UdfGroupIds& udfGroupIds) {
    auto loadBalancer = makeLoadBalancer(
        id, v4Fields, v6Fields, transportFields, mplsFields, udfGroupIds);

    auto loadBalancers = getProgrammedState()->getLoadBalancers()->clone();
    if (!loadBalancers->getNodeIf(id)) {
      loadBalancers->addNode(std::move(loadBalancer));
    } else {
      loadBalancers->updateNode(std::move(loadBalancer));
    }
    auto state = getProgrammedState();
    state->modify(&state);
    state->resetLoadBalancers(std::move(loadBalancers));
    return state;
  }

  std::unique_ptr<LoadBalancer> makeLoadBalancer(
      LoadBalancerID id,
      const LoadBalancer::IPv4Fields& v4Fields,
      const LoadBalancer::IPv6Fields& v6Fields,
      const LoadBalancer::TransportFields& transportFields,
      const LoadBalancer::MPLSFields& mplsFields,
      const LoadBalancer::UdfGroupIds& udfGroupIds) {
    return std::make_unique<LoadBalancer>(
        id,
        cfg::HashingAlgorithm::CRC16_CCITT,
        getSeed(id),
        v4Fields,
        v6Fields,
        transportFields,
        mplsFields,
        udfGroupIds);
  }

  uint32_t getDefaultHashSubFieldSelectorsForL3MPLSForward() {
    return BCM_HASH_MPLS_FIELD_TOP_LABEL | BCM_HASH_MPLS_FIELD_2ND_LABEL |
        BCM_HASH_MPLS_FIELD_3RD_LABEL | BCM_HASH_MPLS_FIELD_LABELS_4MSB |
        BCM_HASH_MPLS_FIELD_IP4SRC_LO | BCM_HASH_MPLS_FIELD_IP4SRC_HI |
        BCM_HASH_MPLS_FIELD_IP4DST_LO | BCM_HASH_MPLS_FIELD_IP4DST_HI;
  }

  uint32_t getDefaultHashSubFieldSelectorsForL3MPLSTerminate() {
    return BCM_HASH_MPLS_FIELD_IP4SRC_LO | BCM_HASH_MPLS_FIELD_IP4SRC_HI |
        BCM_HASH_MPLS_FIELD_IP4DST_LO | BCM_HASH_MPLS_FIELD_IP4DST_HI |
        BCM_HASH_FIELD_FLOWLABEL_LO | BCM_HASH_FIELD_FLOWLABEL_HI |
        BCM_HASH_FIELD_SRCL4 | BCM_HASH_FIELD_DSTL4;
  }

  inline uint32_t getSeed(LoadBalancerID id) const {
    return id == LoadBalancerID::AGGREGATE_PORT ? kTrunkSeed : kEcmpSeed;
  }

  const LoadBalancer::IPv4Fields kV4Fields_{
      LoadBalancer::IPv4Field::SOURCE_ADDRESS,
      LoadBalancer::IPv4Field::DESTINATION_ADDRESS};

  const LoadBalancer::IPv6Fields kV6Fields_{
      LoadBalancer::IPv6Field::SOURCE_ADDRESS,
      LoadBalancer::IPv6Field::DESTINATION_ADDRESS,
      LoadBalancer::IPv6Field::FLOW_LABEL};

  const LoadBalancer::IPv6Fields kV6FieldsNoFlowLabel_{
      LoadBalancer::IPv6Field::SOURCE_ADDRESS,
      LoadBalancer::IPv6Field::DESTINATION_ADDRESS};

  const LoadBalancer::TransportFields kTransportFields_{
      LoadBalancer::TransportField::SOURCE_PORT,
      LoadBalancer::TransportField::DESTINATION_PORT};

  const LoadBalancer::MPLSFields kMplsFields_{
      LoadBalancer::MPLSField::TOP_LABEL,
      LoadBalancer::MPLSField::SECOND_LABEL,
      LoadBalancer::MPLSField::THIRD_LABEL};

  const LoadBalancer::UdfGroupIds kNoUdfGroupIds_{};
  const LoadBalancer::UdfGroupIds kUdfGroupIds_{"dstQueuePair"};
  static constexpr auto kEcmpSeed = 0x7E57EDA;
  static constexpr auto kTrunkSeed = 0x7E57EDB;
};

TEST_F(BcmRtag7Test, programUdfLoadBalancerMap) {
  auto setupUdfLoadBalancers = [=]() { applyNewState(setupFullEcmpUdfHash()); };
  auto verifyUdfLoadBalancers = [=]() {
    const int v4HalfHash = BCM_HASH_FIELD_IP4SRC_LO | BCM_HASH_FIELD_IP4SRC_HI |
        BCM_HASH_FIELD_IP4DST_LO | BCM_HASH_FIELD_IP4DST_HI;
    const int v4FullHash =
        v4HalfHash | BCM_HASH_FIELD_SRCL4 | BCM_HASH_FIELD_DSTL4;
    const int udfV4FullHash = v4FullHash | kUdfHashFields;

    const int v6HalfHash = BCM_HASH_FIELD_IP6SRC_LO | BCM_HASH_FIELD_IP6SRC_HI |
        BCM_HASH_FIELD_IP6DST_LO | BCM_HASH_FIELD_IP6DST_HI;
    const int v6FullHash =
        v6HalfHash | BCM_HASH_FIELD_SRCL4 | BCM_HASH_FIELD_DSTL4;
    const int udfV6FullHash = v6FullHash | kUdfHashFields;

    utility::assertSwitchControl(
        bcmSwitchHashIP4TcpUdpPortsEqualField0, udfV4FullHash);
    utility::assertSwitchControl(bcmSwitchHashIP6Field0, udfV6FullHash);
    utility::assertSwitchControl(bcmSwitchHashIP4TcpUdpField0, udfV4FullHash);
    utility::assertSwitchControl(bcmSwitchHashIP6TcpUdpField0, udfV6FullHash);
    utility::assertSwitchControl(
        bcmSwitchHashIP4TcpUdpPortsEqualField0, udfV4FullHash);
    utility::assertSwitchControl(
        bcmSwitchHashIP6TcpUdpPortsEqualField0, udfV6FullHash);
    // ensure udf hash is enabled
    utility::assertSwitchControl(
        bcmSwitchUdfHashEnable, BCM_HASH_FIELD0_ENABLE_UDFHASH);
  };

  setupUdfLoadBalancers();
  verifyUdfLoadBalancers();
};

TEST_F(BcmRtag7Test, unprogramUdfLoadBalancerMap) {
  auto setupUndoUdfLoadBalancers = [=]() {
    applyNewState(setupFullEcmpUdfHash());
    // undo udf hashing
    applyNewState(setupFullEcmpHash());
  };
  auto verifyUndoUdfLoadBalancers = [=]() {
    const int v4HalfHash = BCM_HASH_FIELD_IP4SRC_LO | BCM_HASH_FIELD_IP4SRC_HI |
        BCM_HASH_FIELD_IP4DST_LO | BCM_HASH_FIELD_IP4DST_HI;
    const int v4FullHash =
        v4HalfHash | BCM_HASH_FIELD_SRCL4 | BCM_HASH_FIELD_DSTL4;

    const int v6HalfHash = BCM_HASH_FIELD_IP6SRC_LO | BCM_HASH_FIELD_IP6SRC_HI |
        BCM_HASH_FIELD_IP6DST_LO | BCM_HASH_FIELD_IP6DST_HI;
    const int v6FullHash =
        v6HalfHash | BCM_HASH_FIELD_SRCL4 | BCM_HASH_FIELD_DSTL4;

    utility::assertSwitchControl(
        bcmSwitchHashIP4TcpUdpPortsEqualField0, v4FullHash);
    utility::assertSwitchControl(bcmSwitchHashIP6Field0, v6FullHash);
    utility::assertSwitchControl(bcmSwitchHashIP4TcpUdpField0, v4FullHash);
    utility::assertSwitchControl(bcmSwitchHashIP6TcpUdpField0, v6FullHash);
    utility::assertSwitchControl(
        bcmSwitchHashIP4TcpUdpPortsEqualField0, v4FullHash);
    utility::assertSwitchControl(
        bcmSwitchHashIP6TcpUdpPortsEqualField0, v6FullHash);
    // ensure udf hash is disabled
    utility::assertSwitchControl(bcmSwitchUdfHashEnable, 0);
  };

  setupUndoUdfLoadBalancers();
  verifyUndoUdfLoadBalancers();
};

TEST_F(BcmRtag7Test, programLoadBalancerMap) {
  auto setupBothLoadBalancers = [=]() {
    applyNewState(setupFullEcmpHash());
    applyNewState(setupHalfTrunkHash());
  };

  auto verifyBothLoadBalancers = [=]() {
    // TODO(samank): factor out unit number
    const bool kEnable = true;

    const int v4HalfHash = BCM_HASH_FIELD_IP4SRC_LO | BCM_HASH_FIELD_IP4SRC_HI |
        BCM_HASH_FIELD_IP4DST_LO | BCM_HASH_FIELD_IP4DST_HI;
    const int v4FullHash =
        v4HalfHash | BCM_HASH_FIELD_SRCL4 | BCM_HASH_FIELD_DSTL4;

    const int v6HalfHash = BCM_HASH_FIELD_IP6SRC_LO | BCM_HASH_FIELD_IP6SRC_HI |
        BCM_HASH_FIELD_IP6DST_LO | BCM_HASH_FIELD_IP6DST_HI;
    const int v6FullHash =
        v6HalfHash | BCM_HASH_FIELD_SRCL4 | BCM_HASH_FIELD_DSTL4;
    // ECMP load-balancing settings
    utility::assertSwitchControl(bcmSwitchHashField0PreProcessEnable, kEnable);
    utility::assertSwitchControl(
        bcmSwitchHashSeed0, getSeed(LoadBalancerID::ECMP));
    utility::assertSwitchControl(bcmSwitchHashIP4Field0, v4FullHash);
    utility::assertSwitchControl(bcmSwitchHashIP6Field0, v6FullHash);
    utility::assertSwitchControl(bcmSwitchHashIP4TcpUdpField0, v4FullHash);
    utility::assertSwitchControl(bcmSwitchHashIP6TcpUdpField0, v6FullHash);
    utility::assertSwitchControl(
        bcmSwitchHashIP4TcpUdpPortsEqualField0, v4FullHash);
    utility::assertSwitchControl(
        bcmSwitchHashIP6TcpUdpPortsEqualField0, v6FullHash);
    utility::assertSwitchControl(
        bcmSwitchHashField0Config, BCM_HASH_FIELD_CONFIG_CRC16CCITT);
    utility::assertSwitchControl(
        bcmSwitchHashField0Config1, BCM_HASH_FIELD_CONFIG_CRC16CCITT);
    // ECMP output-selection settings
    utility::assertSwitchControl(bcmSwitchHashUseFlowSelEcmp, 1);
    utility::assertSwitchControl(bcmSwitchMacroFlowHashMinOffset, 0);
    utility::assertSwitchControl(bcmSwitchMacroFlowHashMaxOffset, 15);
    utility::assertSwitchControl(bcmSwitchMacroFlowHashStrideOffset, 1);
    // ECMP specific settings
    utility::assertSwitchControl(
        bcmSwitchHashControl, BCM_HASH_CONTROL_ECMP_ENHANCE);

    // LAG load-balancing settings
    utility::assertSwitchControl(bcmSwitchHashField1PreProcessEnable, kEnable);
    utility::assertSwitchControl(
        bcmSwitchHashSeed1, getSeed(LoadBalancerID::AGGREGATE_PORT));
    utility::assertSwitchControl(bcmSwitchHashIP4Field1, v4HalfHash);
    utility::assertSwitchControl(bcmSwitchHashIP6Field1, v6HalfHash);
    utility::assertSwitchControl(bcmSwitchHashIP4TcpUdpField1, v4HalfHash);
    utility::assertSwitchControl(bcmSwitchHashIP6TcpUdpField1, v6HalfHash);
    utility::assertSwitchControl(
        bcmSwitchHashIP4TcpUdpPortsEqualField1, v4HalfHash);
    utility::assertSwitchControl(
        bcmSwitchHashIP6TcpUdpPortsEqualField1, v6HalfHash);
    utility::assertSwitchControl(
        bcmSwitchHashField1Config, BCM_HASH_FIELD_CONFIG_CRC16CCITT);
    utility::assertSwitchControl(
        bcmSwitchHashField1Config1, BCM_HASH_FIELD_CONFIG_CRC16CCITT);
    // LAG output-selection settings
    utility::assertSwitchControl(bcmSwitchHashUseFlowSelTrunkUc, 1);
    if (!getHwSwitch()->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::NON_UNICAST_HASH)) {
      utility::assertSwitchControl(bcmSwitchMacroFlowTrunkHashMinOffset, 16);
      utility::assertSwitchControl(bcmSwitchMacroFlowTrunkHashMaxOffset, 31);
      utility::assertSwitchControl(bcmSwitchMacroFlowTrunkHashStrideOffset, 1);
      utility::assertSwitchControl(bcmSwitchMacroFlowTrunkHashConcatEnable, 0);
    } else {
      utility::assertSwitchControl(
          bcmSwitchMacroFlowTrunkUnicastHashMinOffset, 16);
      utility::assertSwitchControl(
          bcmSwitchMacroFlowTrunkUnicastHashMaxOffset, 31);
      utility::assertSwitchControl(
          bcmSwitchMacroFlowTrunkUnicastHashStrideOffset, 1);
      utility::assertSwitchControl(
          bcmSwitchMacroFlowTrunkUnicastHashConcatEnable, 0);
      utility::assertSwitchControl(
          bcmSwitchMacroFlowTrunkNonUnicastHashMinOffset, 16);
      utility::assertSwitchControl(
          bcmSwitchMacroFlowTrunkNonUnicastHashMaxOffset, 31);
      utility::assertSwitchControl(
          bcmSwitchMacroFlowTrunkNonUnicastHashStrideOffset, 1);
      utility::assertSwitchControl(
          bcmSwitchMacroFlowTrunkNonUnicastHashConcatEnable, 0);
    }

    // Global RTAG7 settings
    utility::assertSwitchControl(bcmSwitchHashSelectControl, 0);
    utility::assertSwitchControl(
        bcmSwitchMacroFlowHashFieldConfig,
        BcmRtag7Module::getMacroFlowIDHashingAlgorithm());
    utility::assertSwitchControl(bcmSwitchMacroFlowHashUseMSB, 1);
    utility::assertSwitchControl(bcmSwitchMacroFlowEcmpHashConcatEnable, 0);
  };

  verifyAcrossWarmBoots(setupBothLoadBalancers, verifyBothLoadBalancers);
}

TEST_F(BcmRtag7Test, programMPLSLoadBalancer) {
  auto setupBothLoadBalancers = [=]() {
    applyNewState(setupHashWithAllFields(LoadBalancerID::ECMP));
    return applyNewState(
        setupHashWithAllFields(LoadBalancerID::AGGREGATE_PORT));
  };

  auto verifyBothLoadBalancers = [=]() {
    auto mplsTerminationHash =
        getDefaultHashSubFieldSelectorsForL3MPLSTerminate();
    auto mplsForwardHash = getDefaultHashSubFieldSelectorsForL3MPLSForward();
    // ECMP load-balancing settings
    utility::assertSwitchControl(
        bcmSwitchHashL3MPLSField0, mplsTerminationHash);
    utility::assertSwitchControl(
        bcmSwitchHashL3MPLSField1, mplsTerminationHash);
    utility::assertSwitchControl(
        bcmSwitchHashMPLSTunnelField0, mplsForwardHash);
    utility::assertSwitchControl(
        bcmSwitchHashMPLSTunnelField0, mplsForwardHash);
  };
  verifyAcrossWarmBoots(setupBothLoadBalancers, verifyBothLoadBalancers);
}

} // namespace facebook::fboss
