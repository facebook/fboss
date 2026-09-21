// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include "fboss/agent/if/gen-cpp2/common_types.h"

#pragma once

namespace facebook::fboss {

IpPrefix ipPrefix(const folly::StringPiece ip, int length);

TeFlow
makeFlowKey(const std::string& dstIp, int prefixLength, uint16_t srcPort);

FlowEntry makeFlow(
    const std::string& dstIp,
    const std::string& nhop,
    const std::string& ifname,
    const uint16_t& srcPort,
    const std::string& counterID,
    const int& prefixLength);
} // namespace facebook::fboss
