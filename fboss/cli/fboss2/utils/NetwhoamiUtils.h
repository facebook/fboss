// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once
#include <optional>
#include "configerator/structs/neteng/netwhoami/gen-cpp2/netwhoami_types.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {
std::optional<facebook::netwhoami::NetWhoAmI> getNetWhoAmI(
    const HostInfo& hostInfo);
} // namespace facebook::fboss
