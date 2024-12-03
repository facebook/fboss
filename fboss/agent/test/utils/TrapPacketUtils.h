// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>

#include <set>

namespace facebook::fboss::utility {

void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    PortID port);
void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    const std::set<PortID>& port);
void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    const folly::CIDRNetwork& prefix);
void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    const std::set<folly::CIDRNetwork>& prefixs);
void addTrapPacketAcl(
    const HwAsic* asic,
    cfg::SwitchConfig* config,
    uint16_t l4DstPort);
void addTrapPacketAcl(cfg::SwitchConfig* config, folly::MacAddress mac);

} // namespace facebook::fboss::utility
