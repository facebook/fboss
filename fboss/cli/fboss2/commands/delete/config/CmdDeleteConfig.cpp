// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/delete/config/CmdDeleteConfig.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

// Explicit template instantiation
template void CmdHandler<CmdDeleteConfig, CmdDeleteConfigTraits>::run();

} // namespace facebook::fboss
