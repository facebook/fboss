// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/types.h"

namespace facebook {
namespace fboss {

uint8_t setTxChannelMask(
    const std::set<uint8_t>& tcvrLanes,
    std::optional<uint8_t> userChannelMask,
    bool enable,
    uint8_t txRxDisableData);

} // namespace fboss
} // namespace facebook
