/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/commands/config/CmdConfigAppliedInfo.h"
#include "fboss/cli/fboss2/commands/config/CmdConfigReload.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionCommit.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionDiff.h"

namespace facebook::fboss {

template void
CmdHandler<CmdConfigAppliedInfo, CmdConfigAppliedInfoTraits>::run();
template void CmdHandler<CmdConfigReload, CmdConfigReloadTraits>::run();
template void
CmdHandler<CmdConfigSessionCommit, CmdConfigSessionCommitTraits>::run();
template void
CmdHandler<CmdConfigSessionDiff, CmdConfigSessionDiffTraits>::run();

} // namespace facebook::fboss
