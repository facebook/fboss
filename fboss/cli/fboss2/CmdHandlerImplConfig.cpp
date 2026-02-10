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
// IWYU pragma: begin_keep
// NOLINTBEGIN(misc-include-cleaner)
// @lint-ignore-every CLANGTIDY facebook-unused-include-check
#include "fboss/cli/fboss2/commands/config/CmdConfigAppliedInfo.h"
#include "fboss/cli/fboss2/commands/config/CmdConfigReload.h"
#include "fboss/cli/fboss2/commands/config/history/CmdConfigHistory.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceQueuingPolicy.h"
#include "fboss/cli/fboss2/commands/config/interface/pfc_config/CmdConfigInterfacePfcConfig.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/CmdConfigInterfaceSwitchport.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/access/CmdConfigInterfaceSwitchportAccess.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/access/vlan/CmdConfigInterfaceSwitchportAccessVlan.h"
#include "fboss/cli/fboss2/commands/config/l2/CmdConfigL2.h"
#include "fboss/cli/fboss2/commands/config/l2/learning_mode/CmdConfigL2LearningMode.h"
#include "fboss/cli/fboss2/commands/config/qos/CmdConfigQos.h"
#include "fboss/cli/fboss2/commands/config/qos/buffer_pool/CmdConfigQosBufferPool.h"
#include "fboss/cli/fboss2/commands/config/qos/policy/CmdConfigQosPolicy.h"
#include "fboss/cli/fboss2/commands/config/qos/policy/CmdConfigQosPolicyMap.h"
#include "fboss/cli/fboss2/commands/config/qos/priority_group_policy/CmdConfigQosPriorityGroupPolicy.h"
#include "fboss/cli/fboss2/commands/config/qos/priority_group_policy/CmdConfigQosPriorityGroupPolicyGroupId.h"
#include "fboss/cli/fboss2/commands/config/qos/queuing_policy/CmdConfigQosQueuingPolicy.h"
#include "fboss/cli/fboss2/commands/config/qos/queuing_policy/CmdConfigQosQueuingPolicyQueueId.h"
#include "fboss/cli/fboss2/commands/config/rollback/CmdConfigRollback.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionCommit.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionDiff.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionRebase.h"
#include "fboss/cli/fboss2/commands/config/vlan/CmdConfigVlan.h"
#include "fboss/cli/fboss2/commands/config/vlan/port/CmdConfigVlanPort.h"
#include "fboss/cli/fboss2/commands/config/vlan/port/tagging_mode/CmdConfigVlanPortTaggingMode.h"
#include "fboss/cli/fboss2/commands/config/vlan/static_mac/CmdConfigVlanStaticMac.h"
#include "fboss/cli/fboss2/commands/config/vlan/static_mac/add/CmdConfigVlanStaticMacAdd.h"
#include "fboss/cli/fboss2/commands/config/vlan/static_mac/delete/CmdConfigVlanStaticMacDelete.h"
// NOLINTEND(misc-include-cleaner)
// IWYU pragma: end_keep

namespace facebook::fboss {

template void
CmdHandler<CmdConfigAppliedInfo, CmdConfigAppliedInfoTraits>::run();
template void CmdHandler<CmdConfigReload, CmdConfigReloadTraits>::run();
template void CmdHandler<CmdConfigInterface, CmdConfigInterfaceTraits>::run();
template void CmdHandler<
    CmdConfigInterfaceQueuingPolicy,
    CmdConfigInterfaceQueuingPolicyTraits>::run();
template void CmdHandler<
    CmdConfigInterfacePfcConfig,
    CmdConfigInterfacePfcConfigTraits>::run();
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
template void
CmdHandler<CmdConfigSessionRebase, CmdConfigSessionRebaseTraits>::run();
template void CmdHandler<CmdConfigL2, CmdConfigL2Traits>::run();
template void
CmdHandler<CmdConfigL2LearningMode, CmdConfigL2LearningModeTraits>::run();
template void CmdHandler<CmdConfigQos, CmdConfigQosTraits>::run();
template void
CmdHandler<CmdConfigQosBufferPool, CmdConfigQosBufferPoolTraits>::run();
template void CmdHandler<CmdConfigQosPolicy, CmdConfigQosPolicyTraits>::run();
template void
CmdHandler<CmdConfigQosPolicyMap, CmdConfigQosPolicyMapTraits>::run();
template void CmdHandler<CmdConfigVlan, CmdConfigVlanTraits>::run();
template void CmdHandler<CmdConfigVlanPort, CmdConfigVlanPortTraits>::run();
template void CmdHandler<
    CmdConfigVlanPortTaggingMode,
    CmdConfigVlanPortTaggingModeTraits>::run();
template void
CmdHandler<CmdConfigVlanStaticMac, CmdConfigVlanStaticMacTraits>::run();
template void
CmdHandler<CmdConfigVlanStaticMacAdd, CmdConfigVlanStaticMacAddTraits>::run();
template void CmdHandler<
    CmdConfigVlanStaticMacDelete,
    CmdConfigVlanStaticMacDeleteTraits>::run();
template void CmdHandler<
    CmdConfigQosPriorityGroupPolicy,
    CmdConfigQosPriorityGroupPolicyTraits>::run();
template void CmdHandler<
    CmdConfigQosPriorityGroupPolicyGroupId,
    CmdConfigQosPriorityGroupPolicyGroupIdTraits>::run();
template void
CmdHandler<CmdConfigQosQueuingPolicy, CmdConfigQosQueuingPolicyTraits>::run();
template void CmdHandler<
    CmdConfigQosQueuingPolicyQueueId,
    CmdConfigQosQueuingPolicyQueueIdTraits>::run();

} // namespace facebook::fboss
