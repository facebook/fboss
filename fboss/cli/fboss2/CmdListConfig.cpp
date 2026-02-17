/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <algorithm>
#include "fboss/cli/fboss2/CmdList.h"

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
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionClear.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionCommit.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionDiff.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionRebase.h"
#include "fboss/cli/fboss2/commands/config/vlan/CmdConfigVlan.h"
#include "fboss/cli/fboss2/commands/config/vlan/port/CmdConfigVlanPort.h"
#include "fboss/cli/fboss2/commands/config/vlan/port/tagging_mode/CmdConfigVlanPortTaggingMode.h"
#include "fboss/cli/fboss2/commands/config/vlan/static_mac/CmdConfigVlanStaticMac.h"
#include "fboss/cli/fboss2/commands/config/vlan/static_mac/add/CmdConfigVlanStaticMacAdd.h"
#include "fboss/cli/fboss2/commands/config/vlan/static_mac/delete/CmdConfigVlanStaticMacDelete.h"

namespace facebook::fboss {

const CommandTree& kConfigCommandTree() {
  static CommandTree root = {
      {"config",
       "applied-info",
       "Show config applied information",
       commandHandler<CmdConfigAppliedInfo>,
       argTypeHandler<CmdConfigAppliedInfoTraits>},

      {"config",
       "history",
       "Show history of committed config revisions",
       commandHandler<CmdConfigHistory>,
       argTypeHandler<CmdConfigHistoryTraits>},

      {
          "config",
          "interface",
          "Configure interface settings",
          commandHandler<CmdConfigInterface>,
          argTypeHandler<CmdConfigInterfaceTraits>,
          {{
               "pfc-config",
               "Configure PFC settings for interface",
               commandHandler<CmdConfigInterfacePfcConfig>,
               argTypeHandler<CmdConfigInterfacePfcConfigTraits>,
           },
           {
               "queuing-policy",
               "Set queuing policy for interface",
               commandHandler<CmdConfigInterfaceQueuingPolicy>,
               argTypeHandler<CmdConfigInterfaceQueuingPolicyTraits>,
           },
           {
               "switchport",
               "Configure switchport settings",
               commandHandler<CmdConfigInterfaceSwitchport>,
               argTypeHandler<CmdConfigInterfaceSwitchportTraits>,
               {{
                   "access",
                   "Configure access mode settings",
                   commandHandler<CmdConfigInterfaceSwitchportAccess>,
                   argTypeHandler<CmdConfigInterfaceSwitchportAccessTraits>,
                   {{
                       "vlan",
                       "Set access VLAN (ingressVlan) for the interface",
                       commandHandler<CmdConfigInterfaceSwitchportAccessVlan>,
                       argTypeHandler<
                           CmdConfigInterfaceSwitchportAccessVlanTraits>,
                   }},
               }},
           }},
      },

      {
          "config",
          "l2",
          "Configure L2 settings",
          commandHandler<CmdConfigL2>,
          argTypeHandler<CmdConfigL2Traits>,
          {{
              "learning-mode",
              "Set L2 learning mode (hardware, software, or disabled)",
              commandHandler<CmdConfigL2LearningMode>,
              argTypeHandler<CmdConfigL2LearningModeTraits>,
          }},
      },

      {
          "config",
          "qos",
          "Configure QoS settings",
          commandHandler<CmdConfigQos>,
          argTypeHandler<CmdConfigQosTraits>,
          {{
               "buffer-pool",
               "Configure buffer pool settings",
               commandHandler<CmdConfigQosBufferPool>,
               argTypeHandler<CmdConfigQosBufferPoolTraits>,
           },
           {
               "policy",
               "Configure QoS policy settings",
               commandHandler<CmdConfigQosPolicy>,
               argTypeHandler<CmdConfigQosPolicyTraits>,
               {{
                   "map",
                   "Set QoS map entry (tc-to-queue, pfc-pri-to-queue, tc-to-pg, pfc-pri-to-pg)",
                   commandHandler<CmdConfigQosPolicyMap>,
                   argTypeHandler<CmdConfigQosPolicyMapTraits>,
               }},
           },
           {
               "priority-group-policy",
               "Configure priority group policy settings",
               commandHandler<CmdConfigQosPriorityGroupPolicy>,
               argTypeHandler<CmdConfigQosPriorityGroupPolicyTraits>,
               {{"group-id",
                 "Specify priority group ID (0-7)",
                 commandHandler<CmdConfigQosPriorityGroupPolicyGroupId>,
                 argTypeHandler<CmdConfigQosPriorityGroupPolicyGroupIdTraits>}},
           },
           {
               "queuing-policy",
               "Configure queuing policy settings",
               commandHandler<CmdConfigQosQueuingPolicy>,
               argTypeHandler<CmdConfigQosQueuingPolicyTraits>,
               {{
                   "queue-id",
                   "Specify queue ID and attributes",
                   commandHandler<CmdConfigQosQueuingPolicyQueueId>,
                   argTypeHandler<CmdConfigQosQueuingPolicyQueueIdTraits>,
               }},
           }},
      },

      {
          "config",
          "session",
          "Manage config session",
          {{
               "clear",
               "Clear the current config session",
               commandHandler<CmdConfigSessionClear>,
               argTypeHandler<CmdConfigSessionClearTraits>,
           },
           {
               "commit",
               "Commit the current config session",
               commandHandler<CmdConfigSessionCommit>,
               argTypeHandler<CmdConfigSessionCommitTraits>,
           },
           {
               "diff",
               "Show diff between configs (session vs live, session vs revision, or revision vs revision)",
               commandHandler<CmdConfigSessionDiff>,
               argTypeHandler<CmdConfigSessionDiffTraits>,
           },
           {
               "rebase",
               "Rebase session changes onto current HEAD",
               commandHandler<CmdConfigSessionRebase>,
               argTypeHandler<CmdConfigSessionRebaseTraits>,
           }},
      },

      {"config",
       "reload",
       "Reload agent configuration",
       commandHandler<CmdConfigReload>,
       argTypeHandler<CmdConfigReloadTraits>},

      {"config",
       "rollback",
       "Rollback to a previous config revision",
       commandHandler<CmdConfigRollback>,
       argTypeHandler<CmdConfigRollbackTraits>},

      {
          "config",
          "vlan",
          "Configure VLAN settings",
          commandHandler<CmdConfigVlan>,
          argTypeHandler<CmdConfigVlanTraits>,
          {{
               "port",
               "Configure port settings for a VLAN",
               commandHandler<CmdConfigVlanPort>,
               argTypeHandler<CmdConfigVlanPortTraits>,
               {{
                   "taggingMode",
                   "Set port tagging mode (tagged/untagged) for the VLAN",
                   commandHandler<CmdConfigVlanPortTaggingMode>,
                   argTypeHandler<CmdConfigVlanPortTaggingModeTraits>,
               }},
           },
           {
               "static-mac",
               "Manage static MAC entries for VLANs",
               commandHandler<CmdConfigVlanStaticMac>,
               argTypeHandler<CmdConfigVlanStaticMacTraits>,
               {{
                    "add",
                    "Add a static MAC entry to a VLAN",
                    commandHandler<CmdConfigVlanStaticMacAdd>,
                    argTypeHandler<CmdConfigVlanStaticMacAddTraits>,
                },
                {
                    "delete",
                    "Delete a static MAC entry from a VLAN",
                    commandHandler<CmdConfigVlanStaticMacDelete>,
                    argTypeHandler<CmdConfigVlanStaticMacDeleteTraits>,
                }},
           }},
      },
  };
  sort(root.begin(), root.end());
  return root;
}

} // namespace facebook::fboss
