// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <functional>

namespace facebook::fboss {
class AgentEnsemble;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

// Max ECMP width used by RUN_HW_LOAD_BALANCER_TEST macros
inline constexpr int kSrv6MaxEcmpWidth = 8;

cfg::Srv6Tunnel makeSrv6TunnelConfig(
    const std::string& name,
    InterfaceID interfaceId);

cfg::SwitchConfig srv6EcmpInitialConfig(const AgentEnsemble& ensemble);

// Common ECN verification for SRv6 encap/decap/midpoint tests.
// Disables TX to build congestion, calls sendFloodPackets to inject
// ECN-capable traffic, re-enables TX, then verifies outEcnCounter
// incremented and a trapped packet matches isEcnMarkedPacket.
void verifySrv6EcnMarking(
    AgentEnsemble* ensemble,
    PortID egressPort,
    const std::function<void()>& sendFloodPackets,
    const std::function<
        bool(const folly::IPAddressV6& dstAddr, uint8_t ecnBits)>&
        isEcnMarkedPacket);

} // namespace facebook::fboss::utility
