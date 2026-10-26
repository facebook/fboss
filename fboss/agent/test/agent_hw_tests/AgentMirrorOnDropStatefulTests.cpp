// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropTestBase.h"
#include "fboss/agent/test/utils/PacketSnooper.h"

namespace facebook::fboss {

class AgentMirrorOnDropStatefulTest : public AgentMirrorOnDropTestBase {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::MIRROR_ON_DROP_STATEFUL};
  }

 protected:
  static constexpr auto kSnooperPacketType =
      utility::packetSnooperReceivePacketType::PACKET_TYPE_CHENAB_MOD;
};

} // namespace facebook::fboss
