// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/types.h"

#include <string>
#include "folly/IPAddress.h"
#include "folly/IPAddressV4.h"
#include "folly/IPAddressV6.h"

namespace facebook::fboss::utility {

cfg::MirrorEgressPort getMirrorEgressPort(const std::string& portName);

cfg::MirrorEgressPort getMirrorEgressPort(PortID portID);
cfg::Mirror getSPANMirror(
    const std::string& name,
    PortID portID,
    uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
    bool truncate = false);

cfg::Mirror getSPANMirror(
    const std::string& name,
    const std::string& portName,
    uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
    bool truncate = false);

cfg::GreTunnel getGreTunnelConfig(folly::IPAddress dstAddress);

cfg::SflowTunnel getSflowTunnelConfig(
    folly::IPAddress dstAddress,
    uint16_t sPort,
    uint16_t dPort);

cfg::MirrorTunnel getGREMirrorTunnelConfig(
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress);

cfg::MirrorTunnel getSflowMirrorTunnelConfig(
    folly::IPAddress dstAddress,
    uint16_t sPort,
    uint16_t dPort,
    std::optional<folly::IPAddress> srcAddress);

cfg::Mirror getGREMirror(
    const std::string& name,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress = std::nullopt,
    uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
    bool truncate = false);

cfg::Mirror getGREMirrorWithPort(
    const std::string& name,
    PortID portID,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress = std::nullopt,
    uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
    bool truncate = false);

cfg::Mirror getGREMirrorWithPort(
    const std::string& name,
    const std::string& portName,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress = std::nullopt,
    uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
    bool truncate = false);
cfg::Mirror getSFlowMirror(
    const std::string& name,
    uint16_t sPort,
    uint16_t dPort,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress,
    uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
    bool truncate = false);

cfg::Mirror getSFlowMirrorWithPort(
    const std::string& name,
    PortID portID,
    uint16_t sPort,
    uint16_t dPort,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress,
    uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
    bool truncate = false

);
cfg::Mirror getSFlowMirrorWithPortName(
    const std::string& name,
    const std::string& portName,
    uint16_t sPort,
    uint16_t dPort,
    folly::IPAddress dstAddress,
    std::optional<folly::IPAddress> srcAddress,
    uint8_t dscp = cfg::switch_config_constants::DEFAULT_MIRROR_DSCP_,
    bool truncate = false

);

} // namespace facebook::fboss::utility
