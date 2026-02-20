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

// BGP config commands
template void CmdHandler<CmdConfigProtocol, CmdConfigProtocolTraits>::run();
template void
CmdHandler<CmdConfigProtocolBgp, CmdConfigProtocolBgpTraits>::run();
template void
CmdHandler<CmdConfigProtocolBgpGlobal, CmdConfigProtocolBgpGlobalTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpGlobalRouterId,
    CmdConfigProtocolBgpGlobalRouterIdTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpGlobalLocalAsn,
    CmdConfigProtocolBgpGlobalLocalAsnTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpGlobalHoldTime,
    CmdConfigProtocolBgpGlobalHoldTimeTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpGlobalConfedAsn,
    CmdConfigProtocolBgpGlobalConfedAsnTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpGlobalClusterId,
    CmdConfigProtocolBgpGlobalClusterIdTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpGlobalNetwork6Add,
    CmdConfigProtocolBgpGlobalNetwork6AddTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimit,
    CmdConfigProtocolBgpGlobalSwitchLimitPrefixLimitTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimit,
    CmdConfigProtocolBgpGlobalSwitchLimitTotalPathLimitTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpGlobalSwitchLimitMaxGoldenVips,
    CmdConfigProtocolBgpGlobalSwitchLimitMaxGoldenVipsTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionMode,
    CmdConfigProtocolBgpGlobalSwitchLimitOverloadProtectionModeTraits>::run();

// BGP peer commands
template void
CmdHandler<CmdConfigProtocolBgpPeer, CmdConfigProtocolBgpPeerTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerRemoteAsn,
    CmdConfigProtocolBgpPeerRemoteAsnTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerPeerGroup,
    CmdConfigProtocolBgpPeerPeerGroupTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerHoldTime,
    CmdConfigProtocolBgpPeerHoldTimeTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerDescription,
    CmdConfigProtocolBgpPeerDescriptionTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerRrClient,
    CmdConfigProtocolBgpPeerRrClientTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerNextHopSelf,
    CmdConfigProtocolBgpPeerNextHopSelfTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerLocalAddr,
    CmdConfigProtocolBgpPeerLocalAddrTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerPassive,
    CmdConfigProtocolBgpPeerPassiveTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerIngressPolicy,
    CmdConfigProtocolBgpPeerIngressPolicyTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerEgressPolicy,
    CmdConfigProtocolBgpPeerEgressPolicyTraits>::run();

// BGP peer-group commands
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroup,
    CmdConfigProtocolBgpPeerGroupTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupRemoteAsn,
    CmdConfigProtocolBgpPeerGroupRemoteAsnTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupDescription,
    CmdConfigProtocolBgpPeerGroupDescriptionTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupRrClient,
    CmdConfigProtocolBgpPeerGroupRrClientTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupNextHopSelf,
    CmdConfigProtocolBgpPeerGroupNextHopSelfTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupTimers,
    CmdConfigProtocolBgpPeerGroupTimersTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupTimersHoldTime,
    CmdConfigProtocolBgpPeerGroupTimersHoldTimeTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupTimersKeepalive,
    CmdConfigProtocolBgpPeerGroupTimersKeepaliveTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupTimersOutDelay,
    CmdConfigProtocolBgpPeerGroupTimersOutDelayTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupConfedPeer,
    CmdConfigProtocolBgpPeerGroupConfedPeerTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupIngressPolicy,
    CmdConfigProtocolBgpPeerGroupIngressPolicyTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupEgressPolicy,
    CmdConfigProtocolBgpPeerGroupEgressPolicyTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupMaxRoutes,
    CmdConfigProtocolBgpPeerGroupMaxRoutesTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupV4OverV6Nh,
    CmdConfigProtocolBgpPeerGroupV4OverV6NhTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupDisableIpv4Afi,
    CmdConfigProtocolBgpPeerGroupDisableIpv4AfiTraits>::run();

// New peer commands
template void CmdHandler<
    CmdConfigProtocolBgpPeerNextHop4,
    CmdConfigProtocolBgpPeerNextHop4Traits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerNextHop6,
    CmdConfigProtocolBgpPeerNextHop6Traits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerMaxRoutes,
    CmdConfigProtocolBgpPeerMaxRoutesTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerV4OverV6Nh,
    CmdConfigProtocolBgpPeerV4OverV6NhTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerDisableIpv4Afi,
    CmdConfigProtocolBgpPeerDisableIpv4AfiTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerLinkBandwidth,
    CmdConfigProtocolBgpPeerLinkBandwidthTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerAdvertiseLbw,
    CmdConfigProtocolBgpPeerAdvertiseLbwTraits>::run();

// New peer commands for RSW config compatibility
template void CmdHandler<
    CmdConfigProtocolBgpPeerPeerId,
    CmdConfigProtocolBgpPeerPeerIdTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerType,
    CmdConfigProtocolBgpPeerTypeTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerWarningLimit,
    CmdConfigProtocolBgpPeerWarningLimitTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerWarningOnly,
    CmdConfigProtocolBgpPeerWarningOnlyTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerTimers,
    CmdConfigProtocolBgpPeerTimersTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerTimersKeepalive,
    CmdConfigProtocolBgpPeerTimersKeepaliveTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerTimersOutDelay,
    CmdConfigProtocolBgpPeerTimersOutDelayTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerTimersWithdrawUnprogDelay,
    CmdConfigProtocolBgpPeerTimersWithdrawUnprogDelayTraits>::run();

// New peer-group commands for RSW config compatibility
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupTimersWithdrawUnprogDelay,
    CmdConfigProtocolBgpPeerGroupTimersWithdrawUnprogDelayTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupPeerTag,
    CmdConfigProtocolBgpPeerGroupPeerTagTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupWarningLimit,
    CmdConfigProtocolBgpPeerGroupWarningLimitTraits>::run();
template void CmdHandler<
    CmdConfigProtocolBgpPeerGroupWarningOnly,
    CmdConfigProtocolBgpPeerGroupWarningOnlyTraits>::run();

} // namespace facebook::fboss
