// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <folly/MacAddress.h>

namespace facebook::fboss::utility {

uint32_t generateDeterministicSeed(
    cfg::LoadBalancerID id,
    const folly::MacAddress& mac,
    bool sai);

} // namespace facebook::fboss::utility
