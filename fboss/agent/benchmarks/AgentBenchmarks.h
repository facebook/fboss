// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/AgentEnsemble.h"

namespace facebook::fboss {

void benchmarksMain(int argc, char* argsv, PlatformInitFn initPlatform);

std::unique_ptr<facebook::fboss::AgentEnsemble> createAgentEnsemble(
    AgentEnsembleConfigFn initialConfigFn,
    uint32_t featuresDesired =
        (HwSwitch::FeaturesDesired::PACKET_RX_DESIRED |
         HwSwitch::FeaturesDesired::LINKSCAN_DESIRED));

} // namespace facebook::fboss
