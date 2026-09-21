// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/commands/show/config/CmdShowConfigTraits.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

std::string getConfigHistory(const HostInfo& hostInfo, std::string config_type);

void printShowConfigMaps(
    CmdShowConfigVersionTraits::RetType& printMap,
    std::ostream& out);
} // namespace facebook::fboss
