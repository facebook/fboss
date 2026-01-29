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
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceDescription.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceMtu.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/CmdConfigInterfaceSwitchport.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/access/CmdConfigInterfaceSwitchportAccess.h"
#include "fboss/cli/fboss2/commands/config/interface/switchport/access/vlan/CmdConfigInterfaceSwitchportAccessVlan.h"
#include "fboss/cli/fboss2/commands/config/qos/CmdConfigQos.h"
#include "fboss/cli/fboss2/commands/config/qos/buffer_pool/CmdConfigQosBufferPool.h"
#include "fboss/cli/fboss2/commands/config/qos/priority_group_policy/CmdConfigQosPriorityGroupPolicy.h"
#include "fboss/cli/fboss2/commands/config/qos/priority_group_policy/CmdConfigQosPriorityGroupPolicyGroupId.h"
#include "fboss/cli/fboss2/commands/config/rollback/CmdConfigRollback.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionCommit.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionDiff.h"
#include "fboss/cli/fboss2/commands/config/session/CmdConfigSessionRebase.h"
#include "fboss/cli/fboss2/commands/config/vlan/CmdConfigVlan.h"
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
               "description",
               "Set interface description",
               commandHandler<CmdConfigInterfaceDescription>,
               argTypeHandler<CmdConfigInterfaceDescriptionTraits>,
           },
           {
               "mtu",
               "Set interface MTU",
               commandHandler<CmdConfigInterfaceMtu>,
               argTypeHandler<CmdConfigInterfaceMtuTraits>,
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
           {"priority-group-policy",
            "Configure priority group policy settings",
            commandHandler<CmdConfigQosPriorityGroupPolicy>,
            argTypeHandler<CmdConfigQosPriorityGroupPolicyTraits>,
            {{"group-id",
              "Specify priority group ID (0-7)",
              commandHandler<CmdConfigQosPriorityGroupPolicyGroupId>,
              argTypeHandler<CmdConfigQosPriorityGroupPolicyGroupIdTraits>}}}},
      },

      {
          "config",
          "session",
          "Manage config session",
          {{
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
