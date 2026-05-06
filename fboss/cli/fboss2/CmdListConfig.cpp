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
#include "fboss/cli/fboss2/commands/config/protocol/CmdConfigProtocol.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/CmdConfigProtocolBgp.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobal.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalClusterId.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalConfedAsn.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalHoldTime.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalLocalAsn.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalNetwork6Add.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalRouterId.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalSwitchLimitMaxGoldenVips.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionMode.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimit.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimit.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroup.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupConfedPeer.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupDescription.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupDisableIpv4Afi.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupEgressPolicy.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupIngressPolicy.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupMaxRoutes.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupNextHopSelf.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupPeerTag.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupRemoteAsn.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupRrClient.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimers.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimersHoldTime.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimersKeepalive.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimersOutDelay.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupTimersWithdrawUnprogDelay.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupV4OverV6Nh.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupWarningLimit.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer-group/CmdConfigProtocolBgpPeerGroupWarningOnly.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeer.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerAdvertiseLbw.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerDescription.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerDisableIpv4Afi.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerEgressPolicy.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerHoldTime.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerIngressPolicy.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerLinkBandwidth.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerLocalAddr.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerMaxRoutes.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerNextHop4.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerNextHop6.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerNextHopSelf.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerPassive.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerPeerGroup.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerPeerId.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerRemoteAsn.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerRrClient.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerTimers.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerTimersKeepalive.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerTimersOutDelay.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerTimersWithdrawUnprogDelay.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerType.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerV4OverV6Nh.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerWarningLimit.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/peer/CmdConfigProtocolBgpPeerWarningOnly.h"
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
#include "fboss/cli/fboss2/commands/delete/config/CmdDeleteConfig.h"

namespace facebook::fboss {

const CommandTree& kConfigCommandTree() {
  static CommandTree root = {
      {"config",
       "applied-info",
       "Show config applied information",
       commandHandler<CmdConfigAppliedInfo>,
       argRegistrar<CmdConfigAppliedInfoTraits>},

      {"config",
       "history",
       "Show history of committed config revisions",
       commandHandler<CmdConfigHistory>,
       argRegistrar<CmdConfigHistoryTraits>},

      {
          "config",
          "interface",
          "Configure interface settings",
          commandHandler<CmdConfigInterface>,
          argRegistrar<CmdConfigInterfaceTraits>,
          {{
               "pfc-config",
               "Configure PFC settings for interface",
               commandHandler<CmdConfigInterfacePfcConfig>,
               argRegistrar<CmdConfigInterfacePfcConfigTraits>,
           },
           {
               "queuing-policy",
               "Set queuing policy for interface",
               commandHandler<CmdConfigInterfaceQueuingPolicy>,
               argRegistrar<CmdConfigInterfaceQueuingPolicyTraits>,
           },
           {
               "switchport",
               "Configure switchport settings",
               commandHandler<CmdConfigInterfaceSwitchport>,
               argRegistrar<CmdConfigInterfaceSwitchportTraits>,
               {{
                   "access",
                   "Configure access mode settings",
                   commandHandler<CmdConfigInterfaceSwitchportAccess>,
                   argRegistrar<CmdConfigInterfaceSwitchportAccessTraits>,
                   {{
                       "vlan",
                       "Set access VLAN (ingressVlan) for the interface",
                       commandHandler<CmdConfigInterfaceSwitchportAccessVlan>,
                       argRegistrar<
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
          argRegistrar<CmdConfigL2Traits>,
          {{
              "learning-mode",
              "Set L2 learning mode (hardware, software, or disabled)",
              commandHandler<CmdConfigL2LearningMode>,
              argRegistrar<CmdConfigL2LearningModeTraits>,
          }},
      },

      {
          "config",
          "protocol",
          "Configure protocol settings",
          commandHandler<CmdConfigProtocol>,
          argRegistrar<CmdConfigProtocolTraits>,
          {
              {
                  "bgp",
                  "Configure BGP protocol",
                  commandHandler<CmdConfigProtocolBgp>,
                  argRegistrar<CmdConfigProtocolBgpTraits>,
                  {
                      {
                          "global",
                          "Configure BGP global settings",
                          commandHandler<CmdConfigProtocolBgpGlobal>,
                          argRegistrar<CmdConfigProtocolBgpGlobalTraits>,
                          {
                              {
                                  "router-id",
                                  "Set BGP router identifier",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalRouterId>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpGlobalRouterIdTraits>,
                              },
                              {
                                  "local-asn",
                                  "Set local AS number",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalLocalAsn>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpGlobalLocalAsnTraits>,
                              },
                              {
                                  "hold-time",
                                  "Set BGP hold time in seconds",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalHoldTime>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpGlobalHoldTimeTraits>,
                              },
                              {
                                  "confed-asn",
                                  "Set BGP confederation AS number",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalConfedAsn>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpGlobalConfedAsnTraits>,
                              },
                              {
                                  "cluster-id",
                                  "Set route reflector cluster ID",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalClusterId>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpGlobalClusterIdTraits>,
                              },
                              {
                                  "network6",
                                  "Add IPv6 network to advertise",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalNetwork6Add>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpGlobalNetwork6AddTraits>,
                              },
                              {
                                  "switch-limit",
                                  "Set switch limit prefix-limit",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimit>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimitTraits>,
                              },
                              {
                                  "switch-limit-total-path",
                                  "Set switch limit total-path-limit",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimit>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimitTraits>,
                              },
                              {
                                  "switch-limit-max-golden-vips",
                                  "Set switch limit max-golden-vips",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitMaxGoldenVips>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpGlobalSwitchLimitMaxGoldenVipsTraits>,
                              },
                              {
                                  "switch-limit-overload-protection-mode",
                                  "Set switch limit overload-protection-mode",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionMode>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionModeTraits>,
                              },
                          },
                      },
                      {
                          "peer-group",
                          "Configure BGP peer group",
                          commandHandler<CmdConfigProtocolBgpPeerGroup>,
                          argRegistrar<CmdConfigProtocolBgpPeerGroupTraits>,
                          {
                              {
                                  "description",
                                  "Set peer group description",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupDescription>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupDescriptionTraits>,
                              },
                              {
                                  "remote-asn",
                                  "Set remote AS number",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupRemoteAsn>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupRemoteAsnTraits>,
                              },
                              {
                                  "next-hop-self",
                                  "Enable/disable next-hop-self",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupNextHopSelf>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupNextHopSelfTraits>,
                              },
                              {
                                  "confed-peer",
                                  "Enable/disable confederation peer",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupConfedPeer>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupConfedPeerTraits>,
                              },
                              {
                                  "disable-ipv4-afi",
                                  "Enable/disable IPv4 AFI",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupDisableIpv4Afi>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupDisableIpv4AfiTraits>,
                              },
                              {
                                  "v4-over-v6-nh",
                                  "Enable/disable v4-over-v6 next-hop",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupV4OverV6Nh>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupV4OverV6NhTraits>,
                              },
                              {
                                  "rr-client",
                                  "Enable/disable route reflector client",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupRrClient>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupRrClientTraits>,
                              },
                              {
                                  "ingress-policy",
                                  "Set ingress policy",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupIngressPolicy>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupIngressPolicyTraits>,
                              },
                              {
                                  "egress-policy",
                                  "Set egress policy",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupEgressPolicy>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupEgressPolicyTraits>,
                              },
                              {
                                  "max-routes",
                                  "Set maximum routes",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupMaxRoutes>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupMaxRoutesTraits>,
                              },
                              {
                                  "peer-tag",
                                  "Set peer tag",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupPeerTag>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupPeerTagTraits>,
                              },
                              {
                                  "warning-only",
                                  "Enable/disable warning-only mode",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupWarningOnly>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupWarningOnlyTraits>,
                              },
                              {
                                  "warning-limit",
                                  "Set warning limit percentage",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupWarningLimit>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupWarningLimitTraits>,
                              },
                              {
                                  "timers",
                                  "Configure BGP timers",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupTimers>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerGroupTimersTraits>,
                                  {
                                      {
                                          "hold-time",
                                          "Set hold time",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersHoldTime>,
                                          argRegistrar<
                                              CmdConfigProtocolBgpPeerGroupTimersHoldTimeTraits>,
                                      },
                                      {
                                          "keepalive",
                                          "Set keepalive interval",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersKeepalive>,
                                          argRegistrar<
                                              CmdConfigProtocolBgpPeerGroupTimersKeepaliveTraits>,
                                      },
                                      {
                                          "out-delay",
                                          "Set output delay",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersOutDelay>,
                                          argRegistrar<
                                              CmdConfigProtocolBgpPeerGroupTimersOutDelayTraits>,
                                      },
                                      {
                                          "withdraw-unprog-delay",
                                          "Set withdraw unprogrammed delay",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersWithdrawUnprogDelay>,
                                          argRegistrar<
                                              CmdConfigProtocolBgpPeerGroupTimersWithdrawUnprogDelayTraits>,
                                      },
                                  },
                              },
                          },
                      },
                      {
                          "peer",
                          "Configure BGP peer",
                          commandHandler<CmdConfigProtocolBgpPeer>,
                          argRegistrar<CmdConfigProtocolBgpPeerTraits>,
                          {
                              {
                                  "peer-group",
                                  "Set peer group",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerPeerGroup>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerPeerGroupTraits>,
                              },
                              {
                                  "description",
                                  "Set peer description",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerDescription>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerDescriptionTraits>,
                              },
                              {
                                  "remote-asn",
                                  "Set remote AS number",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerRemoteAsn>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerRemoteAsnTraits>,
                              },
                              {
                                  "local-addr",
                                  "Set local address",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerLocalAddr>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerLocalAddrTraits>,
                              },
                              {
                                  "next-hop4",
                                  "Set IPv4 next-hop",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerNextHop4>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerNextHop4Traits>,
                              },
                              {
                                  "next-hop6",
                                  "Set IPv6 next-hop",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerNextHop6>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerNextHop6Traits>,
                              },
                              {
                                  "next-hop-self",
                                  "Enable/disable next-hop-self",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerNextHopSelf>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerNextHopSelfTraits>,
                              },
                              {
                                  "disable-ipv4-afi",
                                  "Enable/disable IPv4 AFI",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerDisableIpv4Afi>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerDisableIpv4AfiTraits>,
                              },
                              {
                                  "v4-over-v6-nh",
                                  "Enable/disable v4-over-v6 next-hop",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerV4OverV6Nh>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerV4OverV6NhTraits>,
                              },
                              {
                                  "rr-client",
                                  "Enable/disable route reflector client",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerRrClient>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerRrClientTraits>,
                              },
                              {
                                  "passive",
                                  "Enable/disable passive mode",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerPassive>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerPassiveTraits>,
                              },
                              {
                                  "type",
                                  "Set peer type",
                                  commandHandler<CmdConfigProtocolBgpPeerType>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerTypeTraits>,
                              },
                              {
                                  "ingress-policy",
                                  "Set ingress policy",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerIngressPolicy>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerIngressPolicyTraits>,
                              },
                              {
                                  "egress-policy",
                                  "Set egress policy",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerEgressPolicy>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerEgressPolicyTraits>,
                              },
                              {
                                  "max-routes",
                                  "Set maximum routes",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerMaxRoutes>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerMaxRoutesTraits>,
                              },
                              {
                                  "peer-id",
                                  "Set peer identifier",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerPeerId>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerPeerIdTraits>,
                              },
                              {
                                  "advertise-lbw",
                                  "Enable/disable link bandwidth advertisement",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerAdvertiseLbw>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerAdvertiseLbwTraits>,
                              },
                              {
                                  "link-bandwidth",
                                  "Set link bandwidth",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerLinkBandwidth>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerLinkBandwidthTraits>,
                              },
                              {
                                  "hold-time",
                                  "Set hold time",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerHoldTime>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerHoldTimeTraits>,
                              },
                              {
                                  "warning-only",
                                  "Enable/disable warning-only mode",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerWarningOnly>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerWarningOnlyTraits>,
                              },
                              {
                                  "warning-limit",
                                  "Set warning limit percentage",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerWarningLimit>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerWarningLimitTraits>,
                              },
                              {
                                  "timers",
                                  "Configure BGP timers",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerTimers>,
                                  argRegistrar<
                                      CmdConfigProtocolBgpPeerTimersTraits>,
                                  {
                                      {
                                          "keepalive",
                                          "Set keepalive interval",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerTimersKeepalive>,
                                          argRegistrar<
                                              CmdConfigProtocolBgpPeerTimersKeepaliveTraits>,
                                      },
                                      {
                                          "out-delay",
                                          "Set output delay",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerTimersOutDelay>,
                                          argRegistrar<
                                              CmdConfigProtocolBgpPeerTimersOutDelayTraits>,
                                      },
                                      {
                                          "withdraw-unprog-delay",
                                          "Set withdraw unprogrammed delay",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerTimersWithdrawUnprogDelay>,
                                          argRegistrar<
                                              CmdConfigProtocolBgpPeerTimersWithdrawUnprogDelayTraits>,
                                      },
                                  },
                              },
                          },
                      },
                  },
              },
          },
      },

      {
          "config",
          "qos",
          "Configure QoS settings",
          commandHandler<CmdConfigQos>,
          argRegistrar<CmdConfigQosTraits>,
          {{
               "buffer-pool",
               "Configure buffer pool settings",
               commandHandler<CmdConfigQosBufferPool>,
               argRegistrar<CmdConfigQosBufferPoolTraits>,
           },
           {
               "policy",
               "Configure QoS policy settings",
               commandHandler<CmdConfigQosPolicy>,
               argRegistrar<CmdConfigQosPolicyTraits>,
               {{
                   "map",
                   "Set QoS map entry (tc-to-queue, pfc-pri-to-queue, tc-to-pg, pfc-pri-to-pg)",
                   commandHandler<CmdConfigQosPolicyMap>,
                   argRegistrar<CmdConfigQosPolicyMapTraits>,
               }},
           },
           {
               "priority-group-policy",
               "Configure priority group policy settings",
               commandHandler<CmdConfigQosPriorityGroupPolicy>,
               argRegistrar<CmdConfigQosPriorityGroupPolicyTraits>,
               {{"group-id",
                 "Specify priority group ID (0-7)",
                 commandHandler<CmdConfigQosPriorityGroupPolicyGroupId>,
                 argRegistrar<CmdConfigQosPriorityGroupPolicyGroupIdTraits>}},
           },
           {
               "queuing-policy",
               "Configure queuing policy settings",
               commandHandler<CmdConfigQosQueuingPolicy>,
               argRegistrar<CmdConfigQosQueuingPolicyTraits>,
               {{
                   "queue-id",
                   "Specify queue ID and attributes",
                   commandHandler<CmdConfigQosQueuingPolicyQueueId>,
                   argRegistrar<CmdConfigQosQueuingPolicyQueueIdTraits>,
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
               argRegistrar<CmdConfigSessionClearTraits>,
           },
           {
               "commit",
               "Commit the current config session",
               commandHandler<CmdConfigSessionCommit>,
               argRegistrar<CmdConfigSessionCommitTraits>,
           },
           {
               "diff",
               "Show diff between configs (session vs live, session vs revision, or revision vs revision)",
               commandHandler<CmdConfigSessionDiff>,
               argRegistrar<CmdConfigSessionDiffTraits>,
           },
           {
               "rebase",
               "Rebase session changes onto current HEAD",
               commandHandler<CmdConfigSessionRebase>,
               argRegistrar<CmdConfigSessionRebaseTraits>,
           }},
      },

      {"config",
       "reload",
       "Reload agent configuration",
       commandHandler<CmdConfigReload>,
       argRegistrar<CmdConfigReloadTraits>},

      {"config",
       "rollback",
       "Rollback to a previous config revision",
       commandHandler<CmdConfigRollback>,
       argRegistrar<CmdConfigRollbackTraits>},

      {
          "config",
          "vlan",
          "Configure VLAN settings",
          commandHandler<CmdConfigVlan>,
          argRegistrar<CmdConfigVlanTraits>,
          {{
               "port",
               "Configure VLAN port settings",
               commandHandler<CmdConfigVlanPort>,
               argRegistrar<CmdConfigVlanPortTraits>,
               {{
                   "taggingMode",
                   "Set VLAN port tagging mode (tagged, untagged, priority-tagged)",
                   commandHandler<CmdConfigVlanPortTaggingMode>,
                   argRegistrar<CmdConfigVlanPortTaggingModeTraits>,
               }},
           },
           {
               "static-mac",
               "Manage static MAC entries for VLANs",
               commandHandler<CmdConfigVlanStaticMac>,
               argRegistrar<CmdConfigVlanStaticMacTraits>,
               {{
                    "add",
                    "Add a static MAC entry to a VLAN",
                    commandHandler<CmdConfigVlanStaticMacAdd>,
                    argRegistrar<CmdConfigVlanStaticMacAddTraits>,
                },
                {
                    "delete",
                    "Delete a static MAC entry from a VLAN",
                    commandHandler<CmdConfigVlanStaticMacDelete>,
                    argRegistrar<CmdConfigVlanStaticMacDeleteTraits>,
                }},
           }},
      },

      {"delete",
       "config",
       "Delete config objects",
       commandHandler<CmdDeleteConfig>,
       argRegistrar<CmdDeleteConfigTraits>},
  };
  sort(root.begin(), root.end());
  return root;
}

} // namespace facebook::fboss
