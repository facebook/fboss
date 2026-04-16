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
#include "fboss/cli/fboss2/commands/config/interface/ipv6/CmdConfigInterfaceIpv6.h"
#include "fboss/cli/fboss2/commands/config/interface/ipv6/nd/CmdConfigInterfaceIpv6Nd.h"
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
               "ipv6",
               "Configure IPv6 settings for interface",
               commandHandler<CmdConfigInterfaceIpv6>,
               argTypeHandler<CmdConfigInterfaceIpv6Traits>,
               {{
                   "nd",
                   "Configure IPv6 Neighbor Discovery (NDP/RA) settings",
                   commandHandler<CmdConfigInterfaceIpv6Nd>,
                   argTypeHandler<CmdConfigInterfaceIpv6NdTraits>,
               }},
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
          "protocol",
          "Configure protocol settings",
          commandHandler<CmdConfigProtocol>,
          argTypeHandler<CmdConfigProtocolTraits>,
          {
              {
                  "bgp",
                  "Configure BGP protocol",
                  commandHandler<CmdConfigProtocolBgp>,
                  argTypeHandler<CmdConfigProtocolBgpTraits>,
                  {
                      {
                          "global",
                          "Configure BGP global settings",
                          commandHandler<CmdConfigProtocolBgpGlobal>,
                          argTypeHandler<CmdConfigProtocolBgpGlobalTraits>,
                          {
                              {
                                  "router-id",
                                  "Set BGP router identifier",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalRouterId>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpGlobalRouterIdTraits>,
                              },
                              {
                                  "local-asn",
                                  "Set local AS number",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalLocalAsn>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpGlobalLocalAsnTraits>,
                              },
                              {
                                  "hold-time",
                                  "Set BGP hold time in seconds",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalHoldTime>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpGlobalHoldTimeTraits>,
                              },
                              {
                                  "confed-asn",
                                  "Set BGP confederation AS number",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalConfedAsn>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpGlobalConfedAsnTraits>,
                              },
                              {
                                  "cluster-id",
                                  "Set route reflector cluster ID",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalClusterId>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpGlobalClusterIdTraits>,
                              },
                              {
                                  "network6",
                                  "Add IPv6 network to advertise",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalNetwork6Add>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpGlobalNetwork6AddTraits>,
                              },
                              {
                                  "switch-limit",
                                  "Set switch limit prefix-limit",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimit>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimitTraits>,
                              },
                              {
                                  "switch-limit-total-path",
                                  "Set switch limit total-path-limit",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimit>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimitTraits>,
                              },
                              {
                                  "switch-limit-max-golden-vips",
                                  "Set switch limit max-golden-vips",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitMaxGoldenVips>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitMaxGoldenVipsTraits>,
                              },
                              {
                                  "switch-limit-overload-protection-mode",
                                  "Set switch limit overload-protection-mode",
                                  commandHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionMode>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionModeTraits>,
                              },
                          },
                      },
                      {
                          "peer-group",
                          "Configure BGP peer group",
                          commandHandler<CmdConfigProtocolBgpPeerGroup>,
                          argTypeHandler<CmdConfigProtocolBgpPeerGroupTraits>,
                          {
                              {
                                  "description",
                                  "Set peer group description",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupDescription>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupDescriptionTraits>,
                              },
                              {
                                  "remote-asn",
                                  "Set remote AS number",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupRemoteAsn>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupRemoteAsnTraits>,
                              },
                              {
                                  "next-hop-self",
                                  "Enable/disable next-hop-self",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupNextHopSelf>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupNextHopSelfTraits>,
                              },
                              {
                                  "confed-peer",
                                  "Enable/disable confederation peer",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupConfedPeer>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupConfedPeerTraits>,
                              },
                              {
                                  "disable-ipv4-afi",
                                  "Enable/disable IPv4 AFI",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupDisableIpv4Afi>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupDisableIpv4AfiTraits>,
                              },
                              {
                                  "v4-over-v6-nh",
                                  "Enable/disable v4-over-v6 next-hop",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupV4OverV6Nh>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupV4OverV6NhTraits>,
                              },
                              {
                                  "rr-client",
                                  "Enable/disable route reflector client",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupRrClient>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupRrClientTraits>,
                              },
                              {
                                  "ingress-policy",
                                  "Set ingress policy",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupIngressPolicy>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupIngressPolicyTraits>,
                              },
                              {
                                  "egress-policy",
                                  "Set egress policy",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupEgressPolicy>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupEgressPolicyTraits>,
                              },
                              {
                                  "max-routes",
                                  "Set maximum routes",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupMaxRoutes>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupMaxRoutesTraits>,
                              },
                              {
                                  "peer-tag",
                                  "Set peer tag",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupPeerTag>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupPeerTagTraits>,
                              },
                              {
                                  "warning-only",
                                  "Enable/disable warning-only mode",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupWarningOnly>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupWarningOnlyTraits>,
                              },
                              {
                                  "warning-limit",
                                  "Set warning limit percentage",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupWarningLimit>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupWarningLimitTraits>,
                              },
                              {
                                  "timers",
                                  "Configure BGP timers",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerGroupTimers>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerGroupTimersTraits>,
                                  {
                                      {
                                          "hold-time",
                                          "Set hold time",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersHoldTime>,
                                          argTypeHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersHoldTimeTraits>,
                                      },
                                      {
                                          "keepalive",
                                          "Set keepalive interval",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersKeepalive>,
                                          argTypeHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersKeepaliveTraits>,
                                      },
                                      {
                                          "out-delay",
                                          "Set output delay",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersOutDelay>,
                                          argTypeHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersOutDelayTraits>,
                                      },
                                      {
                                          "withdraw-unprog-delay",
                                          "Set withdraw unprogrammed delay",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerGroupTimersWithdrawUnprogDelay>,
                                          argTypeHandler<
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
                          argTypeHandler<CmdConfigProtocolBgpPeerTraits>,
                          {
                              {
                                  "peer-group",
                                  "Set peer group",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerPeerGroup>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerPeerGroupTraits>,
                              },
                              {
                                  "description",
                                  "Set peer description",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerDescription>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerDescriptionTraits>,
                              },
                              {
                                  "remote-asn",
                                  "Set remote AS number",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerRemoteAsn>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerRemoteAsnTraits>,
                              },
                              {
                                  "local-addr",
                                  "Set local address",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerLocalAddr>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerLocalAddrTraits>,
                              },
                              {
                                  "next-hop4",
                                  "Set IPv4 next-hop",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerNextHop4>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerNextHop4Traits>,
                              },
                              {
                                  "next-hop6",
                                  "Set IPv6 next-hop",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerNextHop6>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerNextHop6Traits>,
                              },
                              {
                                  "next-hop-self",
                                  "Enable/disable next-hop-self",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerNextHopSelf>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerNextHopSelfTraits>,
                              },
                              {
                                  "disable-ipv4-afi",
                                  "Enable/disable IPv4 AFI",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerDisableIpv4Afi>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerDisableIpv4AfiTraits>,
                              },
                              {
                                  "v4-over-v6-nh",
                                  "Enable/disable v4-over-v6 next-hop",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerV4OverV6Nh>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerV4OverV6NhTraits>,
                              },
                              {
                                  "rr-client",
                                  "Enable/disable route reflector client",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerRrClient>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerRrClientTraits>,
                              },
                              {
                                  "passive",
                                  "Enable/disable passive mode",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerPassive>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerPassiveTraits>,
                              },
                              {
                                  "type",
                                  "Set peer type",
                                  commandHandler<CmdConfigProtocolBgpPeerType>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerTypeTraits>,
                              },
                              {
                                  "ingress-policy",
                                  "Set ingress policy",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerIngressPolicy>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerIngressPolicyTraits>,
                              },
                              {
                                  "egress-policy",
                                  "Set egress policy",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerEgressPolicy>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerEgressPolicyTraits>,
                              },
                              {
                                  "max-routes",
                                  "Set maximum routes",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerMaxRoutes>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerMaxRoutesTraits>,
                              },
                              {
                                  "peer-id",
                                  "Set peer identifier",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerPeerId>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerPeerIdTraits>,
                              },
                              {
                                  "advertise-lbw",
                                  "Enable/disable link bandwidth advertisement",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerAdvertiseLbw>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerAdvertiseLbwTraits>,
                              },
                              {
                                  "link-bandwidth",
                                  "Set link bandwidth",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerLinkBandwidth>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerLinkBandwidthTraits>,
                              },
                              {
                                  "hold-time",
                                  "Set hold time",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerHoldTime>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerHoldTimeTraits>,
                              },
                              {
                                  "warning-only",
                                  "Enable/disable warning-only mode",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerWarningOnly>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerWarningOnlyTraits>,
                              },
                              {
                                  "warning-limit",
                                  "Set warning limit percentage",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerWarningLimit>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerWarningLimitTraits>,
                              },
                              {
                                  "timers",
                                  "Configure BGP timers",
                                  commandHandler<
                                      CmdConfigProtocolBgpPeerTimers>,
                                  argTypeHandler<
                                      CmdConfigProtocolBgpPeerTimersTraits>,
                                  {
                                      {
                                          "keepalive",
                                          "Set keepalive interval",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerTimersKeepalive>,
                                          argTypeHandler<
                                              CmdConfigProtocolBgpPeerTimersKeepaliveTraits>,
                                      },
                                      {
                                          "out-delay",
                                          "Set output delay",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerTimersOutDelay>,
                                          argTypeHandler<
                                              CmdConfigProtocolBgpPeerTimersOutDelayTraits>,
                                      },
                                      {
                                          "withdraw-unprog-delay",
                                          "Set withdraw unprogrammed delay",
                                          commandHandler<
                                              CmdConfigProtocolBgpPeerTimersWithdrawUnprogDelay>,
                                          argTypeHandler<
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
               "Configure VLAN port settings",
               commandHandler<CmdConfigVlanPort>,
               argTypeHandler<CmdConfigVlanPortTraits>,
               {{
                   "taggingMode",
                   "Set VLAN port tagging mode (tagged, untagged, priority-tagged)",
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

      {"delete",
       "config",
       "Delete config objects",
       commandHandler<CmdDeleteConfig>,
       argTypeHandler<CmdDeleteConfigTraits>},
  };
  sort(root.begin(), root.end());
  return root;
}

} // namespace facebook::fboss
