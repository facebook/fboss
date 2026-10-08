/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <cstdint>

#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss {
class HwAsic;
namespace utility {

/*
 * Utilities for generating a basic, ASIC-aware control-plane policing config.
 * Deployment-specific policies such as role-based MPLS handling, QoS policy
 * attachment, and rollout workarounds remain outside this library.
 */

cfg::Range getRange(uint32_t minimum, uint32_t maximum);
cfg::StreamType getCpuDefaultStreamType(const HwAsic* hwAsic);
cfg::QueueScheduling getCpuDefaultQueueScheduling(const HwAsic* hwAsic);
uint32_t getCoppQueuePps(const HwAsic* hwAsic, uint16_t queueId);
cfg::PortQueueRate getPortQueueRate(const HwAsic* hwAsic, uint16_t queueId);
cfg::ToCpuAction getCpuActionType(const HwAsic* hwAsic);

// Adds high, mid, default, and low CPU queues. Unsupported and non-NPU ASICs
// receive no CPU queue configuration.
void addDefaultCpuQueueConfig(cfg::SwitchConfig& config, const HwAsic& asic);

// Adds common RX-reason mappings and a small IPv6 link-local policing policy.
// CoPP ACLs are inserted directly into the default ingress ACL table; this
// function never writes the legacy flat ACL list.
void addDefaultCpuTrafficPolicyConfig(
    cfg::SwitchConfig& config,
    const HwAsic& asic);

} // namespace utility
} // namespace facebook::fboss
