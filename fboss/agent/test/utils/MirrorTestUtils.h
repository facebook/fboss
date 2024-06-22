// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include "fboss/agent/types.h"

#include <folly/IPAddress.h>

namespace facebook::fboss::utility {

folly::IPAddress getSflowMirrorDestination(bool isV4);

void configureSflowMirror(
    cfg::SwitchConfig& config,
    const std::string& mirrorName,
    bool truncate,
    bool isV4 = true);

void configureSflowSampling(
    cfg::SwitchConfig& config,
    const std::string& mirrorName,
    const std::vector<PortID>& ports,
    int sampleRate);

void configureTrapAcl(cfg::SwitchConfig& config, bool isV4);
void configureTrapAcl(cfg::SwitchConfig& config, PortID portId);

} // namespace facebook::fboss::utility
