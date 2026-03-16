// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

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

} // namespace facebook::fboss::utility
