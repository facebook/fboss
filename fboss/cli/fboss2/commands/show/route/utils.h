// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"

namespace facebook::fboss::show::route::utils {

std::string getMplsActionCodeStr(MplsActionCode mplsActionCode);

std::string getMplsActionInfoStr(const cli::MplsActionInfo& mplsActionInfo);

void getNextHopInfoAddr(
    const network::thrift::BinaryAddress& addr,
    cli::NextHopInfo& nextHopInfo);

void getNextHopInfoThrift(
    const NextHopThrift& nextHop,
    cli::NextHopInfo& nextHopInfo);

std::string getNextHopInfoStr(const cli::NextHopInfo& nextHopInfo);

} // namespace facebook::fboss::show::route::utils
