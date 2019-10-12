// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/state/Route.h"
#include "fboss/agent/types.h"

namespace facebook {
namespace fboss {
class HwSwitchEnsemble;
namespace utility {

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
      std::vector<NextHopWeight>& weights) = 0;

  void pumpTraffic(int ecmpWidth, bool loopThroughFrontPanel);
  void resolveNextHopsandClearStats(unsigned int ecmpWidth);
  void shrinkECMP(unsigned int ecmpWidth);
  void expandECMP(unsigned int ecmpWidth);
  EcmpSetupHelperT* ecmpSetupHelper() const {
    return helper_.get();
  }
  bool isLoadBalanced(
      int ecmpWidth,
      const std::vector<NextHopWeight>& weights,
      uint8_t deviation);

 private:
  virtual void pumpTraffic(folly::Optional<PortID> port) = 0;

  HwSwitchEnsemble* ensemble_;
  std::unique_ptr<EcmpSetupHelperT> helper_;
};

} // namespace utility
} // namespace fboss
} // namespace facebook
