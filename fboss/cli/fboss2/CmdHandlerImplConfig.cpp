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

// Current linter doesn't properly handle the template functions which need the
// following headers
// @lint-ignore-every CLANGTIDY facebook-unused-include-check
#include "fboss/cli/fboss2/commands/config/CmdConfigAppliedInfo.h"
#include "fboss/cli/fboss2/commands/config/CmdConfigReload.h"
#include "fboss/cli/fboss2/commands/config/history/CmdConfigHistory.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceDescription.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceMtu.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/CmdConfigInterfaceSwitchport.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/access/CmdConfigInterfaceSwitchportAccess.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/access/vlan/CmdConfigInterfaceSwitchportAccessVlan.h"
#include "fboss/cli/fboss2/commands/config/qos/CmdConfigQos.h"
#include "fboss/cli/fboss2/commands/config/qos/buffer_pool/CmdConfigQosBufferPool.h"
#include "fboss/cli/fboss2/commands/config/rollback/CmdConfigRollback.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionCommit.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionDiff.h"

namespace facebook::fboss {

template void
CmdHandler<CmdConfigAppliedInfo, CmdConfigAppliedInfoTraits>::run();
template void CmdHandler<CmdConfigReload, CmdConfigReloadTraits>::run();
template void CmdHandler<CmdConfigInterface, CmdConfigInterfaceTraits>::run();
template void CmdHandler<
    CmdConfigInterfaceDescription,
    CmdConfigInterfaceDescriptionTraits>::run();
template void
CmdHandler<CmdConfigInterfaceMtu, CmdConfigInterfaceMtuTraits>::run();
template void CmdHandler<
    CmdConfigInterfaceSwitchport,
    CmdConfigInterfaceSwitchportTraits>::run();
template void CmdHandler<
    CmdConfigInterfaceSwitchportAccess,
    CmdConfigInterfaceSwitchportAccessTraits>::run();
template void CmdHandler<
    CmdConfigInterfaceSwitchportAccessVlan,
    CmdConfigInterfaceSwitchportAccessVlanTraits>::run();
template void CmdHandler<CmdConfigHistory, CmdConfigHistoryTraits>::run();
template void CmdHandler<CmdConfigRollback, CmdConfigRollbackTraits>::run();
template void
CmdHandler<CmdConfigSessionCommit, CmdConfigSessionCommitTraits>::run();
template void
CmdHandler<CmdConfigSessionDiff, CmdConfigSessionDiffTraits>::run();
template void CmdHandler<CmdConfigQos, CmdConfigQosTraits>::run();
template void
CmdHandler<CmdConfigQosBufferPool, CmdConfigQosBufferPoolTraits>::run();

} // namespace facebook::fboss
