// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include "fboss/agent/Utils.h"
#include "fboss/agent/test/AgentHwTest.h"

namespace facebook::fboss::utility {
std::vector<folly::IPAddressV6> getOneRemoteHostIpPerInterfacePort(
    facebook::fboss::AgentEnsemble* ensemble);
void setupEcmpDataplaneLoopOnAllPorts(facebook::fboss::AgentEnsemble* ensemble);
void createTrafficOnMultiplePorts(
    facebook::fboss::AgentEnsemble* ensemble,
    int numberOfPorts,
    std::function<void(
        facebook::fboss::AgentEnsemble* ensemble,
        const folly::IPAddressV6&)> sendPacketFn,
    double desiredPctLineRate = 100);
} // namespace facebook::fboss::utility
