// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/set/sdk/CmdSetSdk.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

// Explicit template instantiation
template void CmdHandler<CmdSetSdk, CmdSetSdkTraits>::run();

} // namespace facebook::fboss
