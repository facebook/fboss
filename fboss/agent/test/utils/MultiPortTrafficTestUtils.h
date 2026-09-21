// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include "fboss/agent/Utils.h"
#include "fboss/agent/test/AgentHwTest.h"

namespace facebook::fboss::utility {
std::vector<folly::IPAddressV6> getOneRemoteHostIpPerInterfacePort(
    facebook::fboss::AgentEnsemble* ensemble);
std::vector<folly::IPAddressV6> getOneRemoteHostIpPerHyperPort(
    facebook::fboss::AgentEnsemble* ensemble);
void setupEcmpDataplaneLoopOnPorts(
    facebook::fboss::AgentEnsemble* ensemble,
    const std::vector<PortID>& ports);
void setupEcmpDataplaneLoopOnAllPorts(facebook::fboss::AgentEnsemble* ensemble);
void createTrafficOnMultiplePorts(
    facebook::fboss::AgentEnsemble* ensemble,
    int numberOfPorts,
    const std::function<void(
        facebook::fboss::AgentEnsemble* ensemble,
        const folly::IPAddressV6&)>& sendPacketFn,
    double desiredPctLineRate = 100);

// Verify that all specified ports reach >= desiredPctLineRate within
// maxRetries iterations (1 second apart). Uses a bulk check: collects
// stats for all pending ports, waits, collects again, and removes
// converged ports. Fails the test if any port doesn't converge.
void waitForLineRateOnPorts(
    facebook::fboss::AgentEnsemble* ensemble,
    const std::vector<PortID>& ports,
    double desiredPctLineRate = 100,
    int maxRetries = 10);
} // namespace facebook::fboss::utility
