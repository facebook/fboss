// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/packet/MPLSHdr.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/types.h"

#include "fboss/agent/test/EcmpSetupHelper.h"

namespace facebook::fboss {
class HwSwitchEnsemble;
}

namespace facebook::fboss::utility {

template <typename EcmpSetupHelperT>
class HwEcmpDataPlaneTestUtil {
 public:
  HwEcmpDataPlaneTestUtil(
      HwSwitchEnsemble* hwSwitchEnsemble,
      std::unique_ptr<EcmpSetupHelperT> helper)
      : ensemble_(hwSwitchEnsemble), helper_(std::move(helper)) {}
  virtual ~HwEcmpDataPlaneTestUtil() {}

  virtual void setupECMPForwarding(
      int ecmpWidth,
      const std::vector<NextHopWeight>& weights) = 0;

  void pumpTraffic(int ecmpWidth, bool loopThroughFrontPanel);
  void unresolveNextHop(unsigned int id);
  void resolveNextHopsandClearStats(unsigned int ecmpWidth);
  void shrinkECMP(unsigned int ecmpWidth);
  void expandECMP(unsigned int ecmpWidth);
  EcmpSetupHelperT* ecmpSetupHelper() const {
    return helper_.get();
  }
  HwSwitchEnsemble* getEnsemble() {
    return ensemble_;
  }
  bool isLoadBalanced(
      int ecmpWidth,
      const std::vector<NextHopWeight>& weights,
      uint8_t deviation);

 private:
  virtual void pumpTrafficThroughPort(std::optional<PortID> port) = 0;

  HwSwitchEnsemble* ensemble_;
  std::unique_ptr<EcmpSetupHelperT> helper_;
};

template <typename AddrT>
class HwIpEcmpDataPlaneTestUtil
    : public HwEcmpDataPlaneTestUtil<EcmpSetupAnyNPorts<AddrT>> {
 public:
  using EcmpSetupAnyNPortsT = EcmpSetupAnyNPorts<AddrT>;
  using BaseT = HwEcmpDataPlaneTestUtil<EcmpSetupAnyNPortsT>;

  HwIpEcmpDataPlaneTestUtil(HwSwitchEnsemble* ensemble, RouterID vrf);

  HwIpEcmpDataPlaneTestUtil(
      HwSwitchEnsemble* ensemble,
      RouterID vrf,
      std::vector<LabelForwardingAction::LabelStack> stacks);

  void setupECMPForwarding(
      int ecmpWidth,
      const std::vector<NextHopWeight>& weights) override;
  /* pump IP traffic */
  void pumpTrafficThroughPort(std::optional<PortID> port) override;

 private:
  std::vector<LabelForwardingAction::LabelStack> stacks_;
};

template <typename AddrT>
class HwMplsEcmpDataPlaneTestUtil
    : public HwEcmpDataPlaneTestUtil<MplsEcmpSetupAnyNPorts<AddrT>> {
 public:
  using EcmpSetupAnyNPortsT = MplsEcmpSetupAnyNPorts<AddrT>;
  using BaseT = HwEcmpDataPlaneTestUtil<MplsEcmpSetupAnyNPorts<AddrT>>;

  HwMplsEcmpDataPlaneTestUtil(
      HwSwitchEnsemble* ensemble,
      MPLSHdr::Label topLabel,
      LabelForwardingAction::LabelForwardingType actionType);

  void setupECMPForwarding(
      int ecmpWidth,
      const std::vector<NextHopWeight>& weights) override;

 private:
  /* pump MPLS traffic */
  void pumpTrafficThroughPort(std::optional<PortID> port) override;
  MPLSHdr::Label label_;
};

using HwIpV4EcmpDataPlaneTestUtil =
    HwIpEcmpDataPlaneTestUtil<folly::IPAddressV4>;
using HwIpV6EcmpDataPlaneTestUtil =
    HwIpEcmpDataPlaneTestUtil<folly::IPAddressV6>;
using HwMplsV4EcmpDataPlaneTestUtil =
    HwMplsEcmpDataPlaneTestUtil<folly::IPAddressV4>;
using HwMplsV6EcmpDataPlaneTestUtil =
    HwMplsEcmpDataPlaneTestUtil<folly::IPAddressV6>;

} // namespace facebook::fboss::utility
