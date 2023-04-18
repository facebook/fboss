/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/Constants.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmControlPlane.h"
#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmExactMatchUtils.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmMirrorUtils.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortQueueManager.h"
#include "fboss/agent/hw/bcm/BcmPtpTcMgr.h"
#include "fboss/agent/hw/bcm/BcmQosMap.h"
#include "fboss/agent/hw/bcm/BcmQosUtils.h"
#include "fboss/agent/hw/bcm/BcmRouteCounter.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/BcmTypes.h"
#include "fboss/agent/hw/bcm/BcmUdfManager.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/LoadBalancerMap.h"
#include "fboss/agent/state/Mirror.h"
#include "fboss/agent/state/MirrorMap.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

extern "C" {
#include <bcm/field.h>
#include <bcm/l3.h>
#include <bcm/mpls.h>
#include <bcm/port.h>
#include <bcm/qos.h>
#include <bcm/trunk.h>
}

using boost::container::flat_map;
using boost::container::flat_set;
using folly::ByteRange;
using folly::IPAddress;
using folly::MacAddress;
using std::make_pair;
using std::make_shared;
using std::make_tuple;
using std::numeric_limits;
using std::shared_ptr;
using std::string;
using std::vector;
using namespace facebook::fboss;
using namespace facebook::fboss::utility;

namespace {
auto constexpr kEcmpObjects = "ecmpObjects";
auto constexpr kTrunks = "trunks";
auto constexpr kVlanForCPUEgressEntries = 0;

struct AddrTables {
  AddrTables()
      : arpTable(make_shared<ArpTable>()), ndpTable(make_shared<NdpTable>()) {}
  shared_ptr<facebook::fboss::ArpTable> arpTable;
  shared_ptr<facebook::fboss::NdpTable> ndpTable;
};

folly::IPAddress getFullMaskIPv4Address() {
  return folly::IPAddress(folly::IPAddressV4(
      folly::IPAddressV4::fetchMask(folly::IPAddressV4::bitCount())));
}

folly::IPAddress getFullMaskIPv6Address() {
  return folly::IPAddress(folly::IPAddressV6(
      folly::IPAddressV6::fetchMask(folly::IPAddressV6::bitCount())));
}
} // namespace

namespace facebook::fboss {

BcmWarmBootCache::BcmWarmBootCache(const BcmSwitchIf* hw)
    : hw_(hw),
      dropEgressId_(BcmEgressBase::INVALID),
      toCPUEgressId_(BcmEgressBase::INVALID),
      bcmWarmBootState_(std::make_unique<BcmWarmBootState>(hw)) {}

folly::dynamic BcmWarmBootCache::getWarmBootStateFollyDynamic() const {
  folly::dynamic bcmWarmBootState = folly::dynamic::object;

  bcmWarmBootState[kHostTable] = bcmWarmBootState_->hostTableToFollyDynamic();
  bcmWarmBootState[kMplsNextHops] =
      bcmWarmBootState_->mplsNextHopsToFollyDynamic();
  bcmWarmBootState[kIntfTable] = bcmWarmBootState_->intfTableToFollyDynamic();
  bcmWarmBootState[kWarmBootCache] = toFollyDynamic();
  bcmWarmBootState[kQosPolicyTable] =
      bcmWarmBootState_->qosTableToFollyDynamic();
  bcmWarmBootState[kTeFlow] = bcmWarmBootState_->teFlowToFollyDynamic();
  bcmWarmBootState[kUdf] = bcmWarmBootState_->udfToFollyDynamic();

  return bcmWarmBootState;
}

template <typename T>
BcmWarmBootCache::EgressId2Weight BcmWarmBootCache::toEgressId2Weight(
    T* egress,
    int count) {
  EgressId2Weight egressIds;
  std::for_each(
      egress, egress + count, [&egressIds](T egress) { egressIds[egress]++; });
  return egressIds;
}

template <>
BcmWarmBootCache::EgressId2Weight BcmWarmBootCache::toEgressId2Weight<
    bcm_l3_ecmp_member_t>(bcm_l3_ecmp_member_t* egress, int count) {
  EgressId2Weight egressIds;
  std::for_each(
      egress, egress + count, [&egressIds](bcm_l3_ecmp_member_t egress) {
        egressIds[egress.egress_if] += egress.weight;
      });
  return egressIds;
}

void BcmWarmBootCache::programmed(AclEntry2AclStatItr itr) {
  XLOG(DBG1) << "Programmed acl stat=" << itr->second.stat;
  itr->second.claimed = true;
}

BcmWarmBootCache::AclEntry2AclStatItr BcmWarmBootCache::findAclStat(
    const BcmAclEntryHandle& bcmAclEntry) {
  auto aclStatItr = aclEntry2AclStat_.find(bcmAclEntry);
  if (aclStatItr != aclEntry2AclStat_.end() && aclStatItr->second.claimed) {
    return aclEntry2AclStat_.end();
  }
  return aclStatItr;
}

void BcmWarmBootCache::programmed(TeFlowEntry2TeFlowStatItr itr) {
  XLOG(DBG1) << "Programmed teflow stat=" << itr->second.stat;
  itr->second.claimed = true;
}

BcmWarmBootCache::TeFlowEntry2TeFlowStatItr BcmWarmBootCache::findTeFlowStat(
    const BcmTeFlowEntryHandle& bcmTeFlowEntry) {
  auto teFlowStatItr = teFlowEntry2TeFlowStat_.find(bcmTeFlowEntry);
  if (teFlowStatItr != teFlowEntry2TeFlowStat_.end() &&
      teFlowStatItr->second.claimed) {
    return teFlowEntry2TeFlowStat_.end();
  }
  return teFlowStatItr;
}

const BcmWarmBootCache::EgressId2Weight& BcmWarmBootCache::getPathsForEcmp(
    EgressId ecmp) const {
  static const EgressId2Weight kEmptyEgressId2Weight;
  if (hwSwitchEcmp2EgressIds_.empty()) {
    // We may have empty hwSwitchEcmp2EgressIds_ when
    // we exited with no ECMP entries.
    return kEmptyEgressId2Weight;
  }
  auto itr = hwSwitchEcmp2EgressIds_.find(ecmp);
  if (itr == hwSwitchEcmp2EgressIds_.end()) {
    throw FbossError("Could not find ecmp ID : ", ecmp);
  }
  return itr->second;
}

folly::dynamic BcmWarmBootCache::toFollyDynamic() const {
  folly::dynamic warmBootCache = folly::dynamic::object;
  // For now we serialize only the hwSwitchEcmp2EgressIds_ table.
  // This is the only thing we need and may not be able to get
  // from HW in the case where we shut down before doing a FIB sync.
  folly::dynamic ecmps = folly::dynamic::array;
  for (auto& ecmpAndEgressIds : hwSwitchEcmp2EgressIds_) {
    folly::dynamic ecmp = folly::dynamic::object;
    ecmp[kEcmpEgressId] = ecmpAndEgressIds.first;
    folly::dynamic paths = folly::dynamic::array;
    for (const auto& path : ecmpAndEgressIds.second) {
      for (int i = 0; i < path.second; i++) {
        paths.push_back(path.first);
      }
    }
    ecmp[kPaths] = std::move(paths);
    ecmps.push_back(std::move(ecmp));
  }
  warmBootCache[kEcmpObjects] = std::move(ecmps);

  folly::dynamic trunks = folly::dynamic::object;
  auto& trunkTable = *getHw()->getTrunkTable();
  for (const auto& e : trunkTable) {
    trunks[std::to_string(e.first)] = e.second->id();
  }
  warmBootCache[kTrunks] = std::move(trunks);

  warmBootCache[BcmRouteCounterTableBase::kRouteCounters] =
      hw_->routeCounterTable()->toFollyDynamic();

  return warmBootCache;
}

void BcmWarmBootCache::populateEcmpEntryFromWarmBootState(
    const folly::dynamic& hwWarmBootState) {
  // Extract ecmps from dumped warm boot cache. We
  // may have shut down before a FIB sync
  auto& ecmpObjects = hwWarmBootState[kWarmBootCache][kEcmpObjects];
  for (const auto& ecmpEntry : ecmpObjects) {
    auto ecmpEgressId = ecmpEntry[kEcmpEgressId].asInt();
    CHECK(ecmpEgressId != BcmEgressBase::INVALID);
    for (const auto& path : ecmpEntry[kPaths]) {
      EgressId e = path.asInt();
      hwSwitchEcmp2EgressIds_[ecmpEgressId][e]++;
    }
  }
  XLOG(DBG1) << "Reconstructed following ecmp path map ";
  for (auto& ecmpIdAndEgress : hwSwitchEcmp2EgressIds_) {
    XLOG(DBG1) << ecmpIdAndEgress.first << " (from warmboot file) ==> "
               << toEgressId2WeightStr(ecmpIdAndEgress.second);
  }
}

void BcmWarmBootCache::populateTrunksFromWarmBootState(
    const folly::dynamic& hwWarmBootState) {
  auto& wbCache = hwWarmBootState[kWarmBootCache];
  if (auto it = wbCache.find(kTrunks); it != wbCache.items().end()) {
    auto& trunks = it->second;
    for (const auto& e : trunks.items()) {
      trunks_[AggregatePortID(e.first.asInt())] = e.second.asInt();
    }
    XLOG(DBG1) << "Reconstructed following list of trunks ";
    for (const auto& e : trunks_) {
      XLOG(DBG0) << "Aggregate port " << e.first << " => trunk ID " << e.second;
    }
  }
}

void BcmWarmBootCache::populateRouteCountersFromWarmBootState(
    const folly::dynamic& hwWarmBootState) {
  auto& wbCache = hwWarmBootState[kWarmBootCache];
  if (wbCache.find(BcmRouteCounterTableBase::kRouteCounters) !=
      wbCache.items().end()) {
    auto& routeCounterInfo = wbCache[BcmRouteCounterTableBase::kRouteCounters];
    if (routeCounterInfo.find(BcmRouteCounterTable::kGlobalModeId) !=
        routeCounterInfo.items().end()) {
      routeCounterModeId_ =
          routeCounterInfo[BcmRouteCounterTable::kGlobalModeId].asInt();
      XLOG(DBG2) << "Found route counter mode id " << routeCounterModeId_;
    }
    if (routeCounterInfo.find(BcmRouteFlexCounterTable::kFlexCounterActionV6) !=
        routeCounterInfo.items().end()) {
      v6FlexCounterAction_ =
          routeCounterInfo[BcmRouteFlexCounterTable::kFlexCounterActionV6]
              .asInt();
      XLOG(DBG2) << "Found v6 route flex counter action id "
                 << v6FlexCounterAction_;
    }
    auto& routeCounterIDs =
        routeCounterInfo[BcmRouteCounterTableBase::kRouteCounterIDs];
    for (const auto& e : routeCounterIDs.items()) {
      auto counterId = e.second;
      if (counterId.type() == folly::dynamic::Type::OBJECT) {
        routeCounterIDs_[e.first.asString()] =
            BcmRouteCounterID::fromFollyDynamic(counterId);
      } else {
        routeCounterIDs_[e.first.asString()] =
            BcmRouteCounterID(counterId.asInt(), 0);
      }
    }
    for (const auto& e : routeCounterIDs_) {
      XLOG(DBG2) << "Found route counter id " << e.first << " Hwid "
                 << e.second.str();
    }
  }
}

void BcmWarmBootCache::populateHostEntryFromWarmBootState(
    const folly::dynamic& hostTable) {
  for (const auto& hostEntry : hostTable[kHosts]) {
    auto egressId = hostEntry[kEgressId].asInt();
    if (egressId == BcmEgressBase::INVALID) {
      continue;
    }
    egressId2WeightInWarmBootFile_[egressId]++;

    std::optional<bcm_if_t> intf{std::nullopt};
    auto ip = folly::IPAddress(hostEntry[kIp].stringPiece());
    if (ip.isV6() && ip.isLinkLocal()) {
      auto egressIt = hostEntry.find(kEgress);
      if (egressIt != hostEntry.items().end()) {
        // check if kIntfId is part of the key, if not. That means it is an ECMP
        // egress object. No interface in this case.
        auto intfIt = egressIt->second.find(kIntfId);
        if (intfIt != egressIt->second.items().end()) {
          intf = intfIt->second.asInt();
        }
      }
    }
    auto vrf = hostEntry[kVrf].asInt();
    auto key = std::make_tuple(vrf, ip, intf);
    vrfIp2EgressFromBcmHostInWarmBootFile_[key] = egressId;

    int classID = 0;
    if (hostEntry.find(kClassID) != hostEntry.items().end()) {
      classID = hostEntry[kClassID].asInt();
    }

    XLOG(DBG1) << "Construct a host entry (vrf=" << vrf << ",ip=" << ip
               << ",intf="
               << (intf.has_value() ? folly::to<std::string>(intf.value())
                                    : "None")
               << ") pointing to the egress entry, id=" << egressId
               << " classID: " << classID;
  }
}

void BcmWarmBootCache::populateMplsNextHopFromWarmBootState(
    const folly::dynamic& hwWarmBootState) {
  const auto& mplsNextHops =
      (hwWarmBootState.find(kMplsNextHops) != hwWarmBootState.items().end())
      ? hwWarmBootState[kMplsNextHops]
      : folly::dynamic::array();

  for (const auto& mplsNextHop : mplsNextHops) {
    auto egressId = mplsNextHop[kEgressId].asInt();
    if (egressId == BcmEgressBase::INVALID) {
      continue;
    }
    egressId2WeightInWarmBootFile_[egressId]++;
    auto vrf = mplsNextHop[kVrf].asInt();
    auto ip = folly::IPAddress(mplsNextHop[kIp].stringPiece());
    auto intfID = InterfaceID(mplsNextHop[kIntf].asInt());
    if (mplsNextHop.items().end() != mplsNextHop.find(kLabel)) {
      // labeled egress
      auto label = mplsNextHop[kLabel].asInt();
      mplsNextHops2EgressIdInWarmBootFile_.emplace(
          BcmLabeledHostKey(vrf, label, ip, intfID), egressId);
    } else {
      // tunneled egress
      CHECK(mplsNextHop.items().end() != mplsNextHop.find(kStack));
      CHECK(mplsNextHop[kStack].isArray());
      LabelForwardingAction::LabelStack labels;
      std::for_each(
          std::begin(mplsNextHop[kStack]),
          std::end(mplsNextHop[kStack]),
          [&labels](const auto& label) { labels.push_back(label.asInt()); });
      mplsNextHops2EgressIdInWarmBootFile_.emplace(
          BcmLabeledHostKey(vrf, std::move(labels), ip, intfID), egressId);
    }
  }
}

void BcmWarmBootCache::populateIntfFromWarmBootState(
    const folly::dynamic& hwWarmBootState) {
  // get l3 intfs for each known vlan in warmboot state file
  // TODO(pshaikh): in earlier warm boot state file, kIntfTable could be
  // absent after two pushes this condition can be removed
  const auto& intfTable =
      (hwWarmBootState.find(kIntfTable) != hwWarmBootState.items().end())
      ? hwWarmBootState[kIntfTable]
      : folly::dynamic::array();
  for (const auto& intfTableEntry : intfTable) {
    vlan2BcmIfIdInWarmBootFile_.emplace(
        VlanID(intfTableEntry[kVlan].asInt()), intfTableEntry[kIntfId].asInt());
  }
}

void BcmWarmBootCache::populateQosPolicyFromWarmBootState(
    const folly::dynamic& hwWarmBootState) {
  // TODO(pshaikh): in earlier warm boot state file, kQosPolicyTable could be
  // absent after two pushes this condition can be removed
  const auto& qosPolicyTable =
      (hwWarmBootState.find(kQosPolicyTable) != hwWarmBootState.items().end())
      ? hwWarmBootState[kQosPolicyTable]
      : folly::dynamic::object();
  for (const auto& qosPolicy : qosPolicyTable.keys()) {
    auto policyName = qosPolicy.asString();
    if (qosPolicyTable[policyName].find(kInDscp) !=
        qosPolicyTable[policyName].items().end()) {
      qosMapKey2QosMapId_.emplace(
          std::make_pair(policyName, BcmQosMap::Type::IP_INGRESS),
          qosPolicyTable[policyName][kInDscp].asInt());
    }
    if (qosPolicyTable[policyName].find(kInExp) !=
        qosPolicyTable[policyName].items().end()) {
      qosMapKey2QosMapId_.emplace(
          std::make_pair(policyName, BcmQosMap::Type::MPLS_INGRESS),
          qosPolicyTable[policyName][kInExp].asInt());
    }
    if (qosPolicyTable[policyName].find(kOutExp) !=
        qosPolicyTable[policyName].items().end()) {
      qosMapKey2QosMapId_.emplace(
          std::make_pair(policyName, BcmQosMap::Type::MPLS_EGRESS),
          qosPolicyTable[policyName][kOutExp].asInt());
    }
  }
}

void BcmWarmBootCache::populateTeFlowFromWarmBootState(
    const folly::dynamic& hwWarmBootState) {
  // Recover Teflow warmboot state when force_init_fp is false
  // force_init_fp is true, switch setting config change will setup Teflow group
  if (!FLAGS_force_init_fp) {
    // get teFlowGroup id and hint id from warmboot state file
    if (hwWarmBootState.find(kTeFlow) != hwWarmBootState.items().end()) {
      const auto& teFlow = hwWarmBootState[kTeFlow];
      teFlowDstIpPrefixLength_ = teFlow[kDstIpPrefixLength].asInt();
      teFlowHintId_ = teFlow[kHintId].asInt();
      teFlowGroupId_ = teFlow[kTeFlowGroupId].asInt();
      teFlowFlexCounterId_ = teFlow[kTeFlowFlexCounterId].asInt();
    }
  }
}

void BcmWarmBootCache::populateUdfGroupFromWarmBootState(
    const folly::dynamic& udfGroup) {
  for (const auto& name : udfGroup.keys()) {
    const auto udfGroupId = udfGroup[name][kUdfGroupIds].asInt();
    /* get udf info */
    bcm_udf_t udfInfo;
    bcm_udf_t_init(&udfInfo);
    auto rv = bcm_udf_get(hw_->getUnit(), udfGroupId, &udfInfo);
    bcmCheckError(rv, "Unable to get udfInfo for udfGroupId: ", udfGroupId);
    XLOG(DBG1) << "udfGroupId=" << udfGroupId
               << " udfInfo layer=" << udfInfo.layer
               << " start=" << udfInfo.start << " width=" << udfInfo.width;
    for (const auto& udfPktMatcherName :
         udfGroup[name][kUdfGroupPktMatchers].keys()) {
      const auto& packetMatcherName = udfPktMatcherName.asString();
      const auto& packetMatcherId =
          udfGroup[name][kUdfGroupPktMatchers][udfPktMatcherName].asInt();
      UdfGroupPacketMatcherMap udfGroupPacketMatcherMap;
      udfGroupPacketMatcherMap.insert({packetMatcherName, packetMatcherId});
      udfGroupNameToPacketMatcherMap_.insert(
          {name.asString(), udfGroupPacketMatcherMap});
      XLOG(DBG2) << "Populating packetMatcher = " << packetMatcherName
                 << ", with id = " << packetMatcherId;
    }
    UdfGroupInfoPair udfGroupInfoPair = {udfGroupId, udfInfo};
    udfGroupNameToInfoMap_.insert({name.asString(), udfGroupInfoPair});
  }
}

void BcmWarmBootCache::populateUdfPacketMatcherFromWarmBootState(
    const folly::dynamic& udfPacketMatcher) {
  for (const auto& name : udfPacketMatcher.keys()) {
    const auto udfPacketMatcherId = udfPacketMatcher[name].asInt();
    /* get udf pkt info */
    bcm_udf_pkt_format_info_t pktFormat;
    bcm_udf_pkt_format_info_t_init(&pktFormat);
    auto rv = bcm_udf_pkt_format_info_get(
        hw_->getUnit(), udfPacketMatcherId, &pktFormat);
    bcmCheckError(
        rv,
        "Unable to get pkt_format for udfPacketMatcherId: ",
        udfPacketMatcherId);
    XLOG(DBG1) << "udfPacketMatcherId=" << udfPacketMatcherId
               << " pkt_format l2=" << pktFormat.l2
               << " ip_protocol=" << pktFormat.ip_protocol
               << " ip_protocol_mask=" << pktFormat.ip_protocol_mask
               << " outer_ip=" << pktFormat.outer_ip
               << " l4_dst_port=" << pktFormat.l4_dst_port;
    UdfPktMatcherInfoPair udfPacketMatcherInfoPair = {
        udfPacketMatcherId, pktFormat};
    udfPktMatcherNameToInfoMap_.insert(
        {name.asString(), udfPacketMatcherInfoPair});
  }
}

void BcmWarmBootCache::populateUdfFromWarmBootState(
    const folly::dynamic& hwWarmBootState) {
  if (hwWarmBootState.find(kUdf) != hwWarmBootState.items().end()) {
    const auto& udfDyanmic = hwWarmBootState[kUdf];
    const auto& udfGroup = udfDyanmic[kUdfGroups];
    populateUdfGroupFromWarmBootState(udfGroup);
    const auto& udfPacketMatcher = udfDyanmic[kUdfPacketMatchers];
    populateUdfPacketMatcherFromWarmBootState(udfPacketMatcher);
    udfEnabled_ = udfDyanmic[kUdfInitState].asBool();
  }
}

void BcmWarmBootCache::populateFromWarmBootState(
    const folly::dynamic& warmBootState,
    std::optional<state::WarmbootState> thriftState) {
  if (thriftState) {
    try {
      dumpedSwSwitchState_ =
          SwitchState::uniquePtrFromThrift(*thriftState->swSwitchState());
    } catch (const FbossError& error) {
      if (!dumpBinaryThriftToFile(
              hw_->getPlatform()->getCrashThriftSwitchStateFile(),
              *thriftState->swSwitchState())) {
        XLOG(ERR) << "failed to dump switch state to file: "
                  << hw_->getPlatform()->getCrashThriftSwitchStateFile();
      } else {
        XLOG(DBG2) << "dumped switch state to file: "
                   << hw_->getPlatform()->getCrashThriftSwitchStateFile();
      }
      XLOG(FATAL) << "Failed to recover switch state from thrift. "
                  << error.what();
    }
  } else {
    XLOG(FATAL) << "Thrift switch state not found";
  }
  dumpedSwSwitchState_->publish();
  CHECK(dumpedSwSwitchState_)
      << "Was not able to recover software state after warmboot";

  auto& hwWarmBootState = warmBootState[kHwSwitch];
  // Extract ecmps for dumped host table
  auto& hostTable = hwWarmBootState[kHostTable];
  for (const auto& ecmpEntry : hostTable[kEcmpHosts]) {
    auto ecmpEgressId = ecmpEntry[kEcmpEgressId].asInt();
    if (ecmpEgressId == BcmEgressBase::INVALID) {
      continue;
    }
    // If the entry is valid, then there must be paths associated with it.
    for (auto path : ecmpEntry[kEcmpEgress][kPaths]) {
      EgressId e = path.asInt();
      hwSwitchEcmp2EgressIds_[ecmpEgressId][e]++;
    }
  }

  populateEcmpEntryFromWarmBootState(hwWarmBootState);

  populateTrunksFromWarmBootState(hwWarmBootState);

  populateRouteCountersFromWarmBootState(hwWarmBootState);

  // Extract BcmHost and its egress object from the warm boot file
  populateHostEntryFromWarmBootState(hostTable);

  // extract MPLS next hop and its egress object from the  warm boot file
  populateMplsNextHopFromWarmBootState(hwWarmBootState);

  populateIntfFromWarmBootState(hwWarmBootState);

  populateQosPolicyFromWarmBootState(hwWarmBootState);

  populateTeFlowFromWarmBootState(hwWarmBootState);

  populateUdfFromWarmBootState(hwWarmBootState);
}

BcmWarmBootCache::EgressId2EgressCitr BcmWarmBootCache::findEgressFromHost(
    bcm_vrf_t vrf,
    const folly::IPAddress& addr,
    std::optional<bcm_if_t> intf) {
  // Do a cheap check of size to avoid construct the key for lookup.
  // That helps the case after warmboot is done.
  if (vrfIp2EgressFromBcmHostInWarmBootFile_.size() == 0) {
    return egressId2Egress_.end();
  }
  // only care about the intf if addr is v6 link-local
  if (!addr.isV6() || !addr.isLinkLocal()) {
    intf = std::nullopt;
  }
  auto key = std::make_tuple(vrf, addr, intf);
  auto it = vrfIp2EgressFromBcmHostInWarmBootFile_.find(key);
  if (it == vrfIp2EgressFromBcmHostInWarmBootFile_.cend()) {
    return egressId2Egress_.end();
  }
  return findEgress(it->second);
}

BcmWarmBootCache::EgressId2EgressCitr
BcmWarmBootCache::findEgressFromLabeledHostKey(const BcmLabeledHostKey& key) {
  // check if this MPLS next hop identified by labeld host key is saved in warm
  // boot state file.
  const auto iter = mplsNextHops2EgressIdInWarmBootFile_.find(key);
  return iter == mplsNextHops2EgressIdInWarmBootFile_.cend()
      ? egressId2Egress_.end()
      : findEgress(iter->second);
}

void BcmWarmBootCache::populate(
    const folly::dynamic& warmBootState,
    std::optional<state::WarmbootState> thriftState) {
  populateFromWarmBootState(warmBootState, thriftState);
  bcm_vlan_data_t* vlanList = nullptr;
  int vlanCount = 0;
  SCOPE_EXIT {
    bcm_vlan_list_destroy(hw_->getUnit(), vlanList, vlanCount);
  };
  auto rv = bcm_vlan_list(hw_->getUnit(), &vlanList, &vlanCount);
  bcmCheckError(rv, "Unable to get vlan information");
  for (auto i = 0; i < vlanCount; ++i) {
    bcm_vlan_data_t& vlanData = vlanList[i];
    int portCount = 0;
    BCM_PBMP_COUNT(vlanData.port_bitmap, portCount);
    XLOG(DBG1) << "Got vlan : " << vlanData.vlan_tag << " with : " << portCount
               << " ports";
    // TODO: Investigate why port_bitmap contains
    // the untagged ports rather than ut_port_bitmap
    vlan2VlanInfo_.insert(make_pair(
        BcmSwitch::getVlanId(vlanData.vlan_tag),
        VlanInfo(
            VlanID(vlanData.vlan_tag),
            vlanData.port_bitmap,
            vlanData.port_bitmap)));
    bcm_l3_intf_t l3Intf;
    bcm_l3_intf_t_init(&l3Intf);
    // there can be more than one interfaces per vlan, such as one l3 intf
    // and other MPLS tunnels. There is no clear way to identify which is
    // which. so keeping track of l3 interfaces in warmboot state file
    // look for l3 intf ID in warmboot state file and try to extract it.
    l3Intf.l3a_vid = vlanData.vlan_tag;
    bool intfFound = false;
    auto iter = vlan2BcmIfIdInWarmBootFile_.find(VlanID(vlanData.vlan_tag));
    if (iter != vlan2BcmIfIdInWarmBootFile_.end()) {
      l3Intf.l3a_intf_id = iter->second;
      l3Intf.l3a_flags = BCM_L3_WITH_ID;
      rv = bcm_l3_intf_get(hw_->getUnit(), &l3Intf);
    } else {
      // this can happen for vlan id 1, a special vlan which is returned by
      // bcm_vlan_list, this vlan has all ports which has no vlan associated
      // it also has port 0 if all front panel ports are associated with some
      // vlan. this vlan won't be in vlan2BcmIfIdInWarmBootFile_
      rv = bcm_l3_intf_find_vlan(hw_->getUnit(), &l3Intf);
    }
    if (rv != BCM_E_NOT_FOUND) {
      bcmCheckError(rv, "failed to find interface for ", vlanData.vlan_tag);
      intfFound = true;
      vlanAndMac2Intf_[make_pair(
          BcmSwitch::getVlanId(l3Intf.l3a_vid),
          macFromBcm(l3Intf.l3a_mac_addr))] = l3Intf;
      XLOG(DBG1) << "Found l3 interface for vlan : " << vlanData.vlan_tag;
    }
    if (intfFound) {
      bcm_l2_station_t l2Station;
      bcm_l2_station_t_init(&l2Station);
      auto stationId =
          hw_->getPlatform()->getAsic()->getStationID(l3Intf.l3a_vid);
      rv = bcm_l2_station_get(hw_->getUnit(), stationId, &l2Station);
      if (!BCM_FAILURE(rv)) {
        XLOG(DBG1) << " Found l2 station with id : " << l3Intf.l3a_vid;
        vlan2Station_[VlanID(vlanData.vlan_tag)] = l2Station;
      } else {
        XLOG(DBG1) << "Could not get l2 station for vlan : "
                   << vlanData.vlan_tag;
      }
    }
  }
  bcm_l3_info_t l3Info;
  bcm_l3_info_t_init(&l3Info);
  bcm_l3_info(hw_->getUnit(), &l3Info);
  if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::HOSTTABLE)) {
    // Traverse V4 hosts
    rv = bcm_l3_host_traverse(
        hw_->getUnit(),
        0,
        0,
        l3Info.l3info_max_host,
        hostTraversalCallback,
        this);
    bcmCheckError(rv, "Failed to traverse v4 hosts");
    // Traverse V6 hosts
    rv = bcm_l3_host_traverse(
        hw_->getUnit(),
        BCM_L3_IP6,
        0,
        // Diag shell uses this for getting # of v6 host entries
        l3Info.l3info_max_host / 2,
        hostTraversalCallback,
        this);
    bcmCheckError(rv, "Failed to traverse v6 hosts");
  }
  // Traverse V4 routes
  rv = bcm_l3_route_traverse(
      hw_->getUnit(),
      0,
      0,
      l3Info.l3info_max_route,
      routeTraversalCallback,
      this);
  bcmCheckError(rv, "Failed to traverse v4 routes");
  // Traverse V6 routes
  rv = bcm_l3_route_traverse(
      hw_->getUnit(),
      BCM_L3_IP6,
      0,
      // Diag shell uses this for getting # of v6 route entries
      l3Info.l3info_max_route / 2,
      routeTraversalCallback,
      this);
  bcmCheckError(rv, "Failed to traverse v6 routes");
  // Get egress entries.
  rv = bcm_l3_egress_traverse(hw_->getUnit(), egressTraversalCallback, this);
  bcmCheckError(rv, "Failed to traverse egress");
  // Traverse ecmp egress entries
  if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::HSDK)) {
    rv = bcm_l3_ecmp_traverse(
        hw_->getUnit(),
        ecmpEgressTraversalCallback<bcm_l3_ecmp_member_t>,
        this);
  } else {
    rv = bcm_l3_egress_ecmp_traverse(
        hw_->getUnit(), ecmpEgressTraversalCallback<bcm_if_t>, this);
  }
  bcmCheckError(rv, "Failed to traverse ecmp egress");

  // populate acls, acl stats
  populateAcls(
      hw_->getPlatform()->getAsic()->getDefaultACLGroupID(),
      this->aclEntry2AclStat_,
      this->priority2BcmAclEntryHandle_);

  // populate teflows, teflow stats
  populateTeFlows(
      teFlowGroupId_,
      this->teFlowEntry2TeFlowStat_,
      this->teFlow2BcmTeFlowEntryHandle_);

  populateRtag7State();
  populateMirrors();
  populateMirroredPorts();
  populateQosMaps();
  populateLabelSwitchActions();
  populateSwitchSettings();
  populateRxReasonToQueue();
}

bool BcmWarmBootCache::fillVlanPortInfo(Vlan* vlan) {
  auto vlanItr = vlan2VlanInfo_.find(vlan->getID());
  if (vlanItr != vlan2VlanInfo_.end()) {
    Vlan::MemberPorts memberPorts;
    bcm_port_t idx;
    BCM_PBMP_ITER(vlanItr->second.untagged, idx) {
      memberPorts.insert(make_pair(PortID(idx), false));
    }
    BCM_PBMP_ITER(vlanItr->second.allPorts, idx) {
      if (memberPorts.find(PortID(idx)) == memberPorts.end()) {
        memberPorts.insert(make_pair(PortID(idx), true));
      }
    }
    vlan->setPorts(memberPorts);
    return true;
  }
  return false;
}

int BcmWarmBootCache::hostTraversalCallback(
    int /*unit*/,
    int /*index*/,
    bcm_l3_host_t* host,
    void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  auto ip = host->l3a_flags & BCM_L3_IP6
      ? IPAddress::fromBinary(
            ByteRange(host->l3a_ip6_addr, sizeof(host->l3a_ip6_addr)))
      : IPAddress::fromLongHBO(host->l3a_ip_addr);
  cache->vrfIp2Host_[make_pair(host->l3a_vrf, ip)] = *host;
  XLOG(DBG1) << "Adding egress id: " << host->l3a_intf << " to " << ip
             << " mapping";
  return 0;
}

int BcmWarmBootCache::egressTraversalCallback(
    int /*unit*/,
    EgressId egressId,
    bcm_l3_egress_t* egress,
    void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  CHECK(cache->egressId2Egress_.find(egressId) == cache->egressId2Egress_.end())
      << "Double callback for egress id: " << egressId;
  // Look up egressId in egressId2WeightInWarmBootFile_
  // to populate both dropEgressId_ and toCPUEgressId_.
  auto egressIdItr = cache->egressId2WeightInWarmBootFile_.find(egressId);
  if (egressIdItr != cache->egressId2WeightInWarmBootFile_.cend()) {
    // May be: Add information to figure out how many host or route entry
    // reference it.
    XLOG(DBG1) << "Adding bcm egress entry for: " << egressIdItr->first
               << " which is referenced by at least one host or route entry.";
    cache->egressId2Egress_[egressId] = *egress;
  } else {
    // found egress ID that is not used by any host entry, we shall
    // only have two of them. One is for drop and the other one is for TO CPU.
    if ((egress->flags & BCM_L3_DST_DISCARD)) {
      if (cache->dropEgressId_ != BcmEgressBase::INVALID) {
        XLOG(FATAL) << "duplicated drop egress found in HW. " << egressId
                    << " and " << cache->dropEgressId_;
      }
      XLOG(DBG1) << "Found drop egress id " << egressId;
      cache->dropEgressId_ = egressId;
    } else if ((egress->flags & (BCM_L3_L2TOCPU | BCM_L3_COPY_TO_CPU))) {
      if (cache->toCPUEgressId_ != BcmEgressBase::INVALID) {
        XLOG(FATAL) << "duplicated generic TO_CPU egress found in HW. "
                    << egressId << " and " << cache->toCPUEgressId_;
      }
      XLOG(DBG1) << "Found generic TO CPU egress id " << egressId;
      cache->toCPUEgressId_ = egressId;
    } else {
      XLOG(FATAL) << "The egress: " << egressId
                  << " is not referenced by any host entry. vlan: "
                  << egress->vlan << " interface: " << egress->intf
                  << " flags: " << std::hex << egress->flags << std::dec;
    }
  }
  cache->populateLabelStack2TunnelId(egress);
  return 0;
}

int BcmWarmBootCache::routeTraversalCallback(
    int /*unit*/,
    int /*index*/,
    bcm_l3_route_t* route,
    void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  bool isIPv6 = route->l3a_flags & BCM_L3_IP6;
  auto ip = isIPv6 ? IPAddress::fromBinary(ByteRange(
                         route->l3a_ip6_net, sizeof(route->l3a_ip6_net)))
                   : IPAddress::fromLongHBO(route->l3a_subnet);
  auto mask = isIPv6 ? IPAddress::fromBinary(ByteRange(
                           route->l3a_ip6_mask, sizeof(route->l3a_ip6_mask)))
                     : IPAddress::fromLongHBO(route->l3a_ip_mask);
  if (cache->getHw()->getPlatform()->canUseHostTableForHostRoutes() &&
      ((isIPv6 && mask == getFullMaskIPv6Address()) ||
       (!isIPv6 && mask == getFullMaskIPv4Address()))) {
    // This is a host route.
    cache->vrfAndIP2Route_[make_pair(route->l3a_vrf, ip)] = *route;
    XLOG(DBG3) << "Adding host route found in route table. vrf: "
               << route->l3a_vrf << " ip: " << ip << " mask: " << mask;
  } else {
    // Other routes that cannot be put into host table / CAM.
    cache->vrfPrefix2Route_[make_tuple(route->l3a_vrf, ip, mask)] = *route;
    XLOG(DBG3) << "In vrf : " << route->l3a_vrf << " adding route for : " << ip
               << " mask: " << mask;
  }
  return 0;
}

template <typename T>
int BcmWarmBootCache::ecmpEgressTraversalCallback(
    int /*unit*/,
    bcm_l3_egress_ecmp_t* ecmp,
    int memberCount,
    T* memberArray,
    void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  EgressId2Weight egressId2Weight;
  // Rather than using the egressId in the memberArray we use the
  // egressId2Weight that we dumped as part of the warm boot state. IntfArray
  // does not include any egressIds that go over the ports that may be
  // down while the warm boot state we dumped does
  try {
    egressId2Weight = cache->getPathsForEcmp(ecmp->ecmp_intf);
  } catch (const FbossError& ex) {
    // There was a bug in SDK where sometimes we got callback with invalid
    // ecmp id with zero number of interfaces. This happened for double wide
    // ECMP entries (when two "words" are used to represent one ECMP entry).
    // For example, if the entries were 200256 and 200258, we got callback
    // for 200257 also with zero interfaces associated with it. If this is
    // the case, we skip this entry.
    //
    // We can also get memberCount of zero with valid ecmp entry (when all the
    // links associated with egress of the ecmp are down. But in this case,
    // cache->getPathsForEcmp() call above should return a valid set of
    // egressIds.
    if (memberCount == 0) {
      return 0;
    }
    throw ex;
  }
  EgressId2Weight egressId2WeightInHw;
  egressId2WeightInHw = cache->toEgressId2Weight<T>(memberArray, memberCount);
  XLOG(DBG1) << "ignoring paths for ecmp egress " << ecmp->ecmp_intf
             << " gotten from hardware: "
             << toEgressId2WeightStr(egressId2WeightInHw);

  CHECK_GT(egressId2Weight.size(), 0)
      << "There must be at least one egress pointed to by the ecmp egress id: "
      << ecmp->ecmp_intf;
  CHECK(
      cache->egressIds2Ecmp_.find(egressId2Weight) ==
      cache->egressIds2Ecmp_.end())
      << "Got a duplicated call for ecmp id: " << ecmp->ecmp_intf
      << " referencing: " << toEgressId2WeightStr(egressId2Weight);
  cache->egressIds2Ecmp_[egressId2Weight] = *ecmp;
  XLOG(DBG1) << "Added ecmp egress id : " << ecmp->ecmp_intf
             << " pointing to : " << toEgressId2WeightStr(egressId2Weight)
             << " egress ids";
  return 0;
}

std::string BcmWarmBootCache::toEgressId2WeightStr(
    const EgressId2Weight& egressId2Weight) {
  std::stringstream ss;
  int i = 1;
  for (const auto& egressId : egressId2Weight) {
    for (int j = 1; j <= egressId.second; j++) {
      ss << egressId.first;
      ss << (i < egressId2Weight.size() || j < egressId.second ? ", " : "");
    }
    i++;
  }
  return ss.str();
}

void BcmWarmBootCache::removeUnclaimedRoutes() {
  for (auto vrfPfxAndRoute : vrfPrefix2Route_) {
    const std::string& routeInfo = folly::to<std::string>(
        "unreferenced route in vrf : ",
        std::get<0>(vrfPfxAndRoute.first),
        " for prefix : ",
        std::get<1>(vrfPfxAndRoute.first),
        "/",
        std::get<2>(vrfPfxAndRoute.first));
    XLOG(DBG1) << "Deleting an " << routeInfo;
    auto rv = bcm_l3_route_delete(hw_->getUnit(), &(vrfPfxAndRoute.second));
    XLOG_IF(WARNING, rv == BCM_E_NOT_FOUND)
        << "Trying to delete a nonexistent " << routeInfo << ", ignore it.";
    if (rv != BCM_E_NOT_FOUND) {
      bcmLogFatal(rv, hw_, "failed to delete an", routeInfo);
    }
  }
  vrfPrefix2Route_.clear();

  for (auto vrfIPAndRoute : vrfAndIP2Route_) {
    const std::string& routeInfo = folly::to<std::string>(
        "fully qualified unreferenced route in vrf:",
        vrfIPAndRoute.first.first,
        " prefix: ",
        vrfIPAndRoute.first.second);
    XLOG(DBG1) << "Deleting a" << routeInfo;
    auto rv = bcm_l3_route_delete(hw_->getUnit(), &(vrfIPAndRoute.second));
    XLOG_IF(WARNING, rv == BCM_E_NOT_FOUND)
        << "Trying to delete a nonexistent " << routeInfo << ", ignore it.";
    if (rv != BCM_E_NOT_FOUND) {
      bcmLogFatal(rv, hw_, "failed to delete a", routeInfo);
    }
  }
  vrfAndIP2Route_.clear();
}

void BcmWarmBootCache::removeUnclaimedHostEntries() {
  for (auto vrfIpAndHost : vrfIp2Host_) {
    XLOG(DBG1) << "Deleting host entry in vrf: " << vrfIpAndHost.first.first
               << " for : " << vrfIpAndHost.first.second;
    auto rv = bcm_l3_host_delete(hw_->getUnit(), &vrfIpAndHost.second);
    bcmLogFatal(
        rv,
        hw_,
        "failed to delete host entry in vrf: ",
        vrfIpAndHost.first.first,
        " for : ",
        vrfIpAndHost.first.second);
  }
  vrfIp2Host_.clear();
}

void BcmWarmBootCache::removeUnclaimedEcmpEgressObjects() {
  for (auto idsAndEcmp : egressIds2Ecmp_) {
    auto& ecmp = idsAndEcmp.second;
    XLOG(DBG1) << "Deleting ecmp egress object  " << ecmp.ecmp_intf
               << " pointing to : " << toEgressId2WeightStr(idsAndEcmp.first);
    int rv;
    if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::HSDK)) {
      rv = bcm_l3_ecmp_destroy(hw_->getUnit(), ecmp.ecmp_intf);
    } else {
      rv = bcm_l3_egress_ecmp_destroy(hw_->getUnit(), &ecmp);
    }
    bcmLogFatal(
        rv,
        hw_,
        "failed to destroy ecmp egress object :",
        ecmp.ecmp_intf,
        " referring to ",
        toEgressId2WeightStr(idsAndEcmp.first));
  }
  egressIds2Ecmp_.clear();
}

void BcmWarmBootCache::removeUnclaimedEgressObjects() {
  for (auto egressIdAndEgress : egressId2Egress_) {
    // This is not used yet
    XLOG(DBG1) << "Deleting egress object: " << egressIdAndEgress.first;
    auto rv = bcm_l3_egress_destroy(hw_->getUnit(), egressIdAndEgress.first);
    bcmLogFatal(
        rv, hw_, "failed to destroy egress object ", egressIdAndEgress.first);
  }
  egressId2Egress_.clear();
}

void BcmWarmBootCache::removeUnclaimedInterfaces() {
  for (auto vlanMacAndIntf : vlanAndMac2Intf_) {
    XLOG(DBG1) << "Deleting l3 interface for vlan: "
               << vlanMacAndIntf.first.first
               << " and mac : " << vlanMacAndIntf.first.second;
    auto rv = bcm_l3_intf_delete(hw_->getUnit(), &vlanMacAndIntf.second);
    bcmLogFatal(
        rv,
        hw_,
        "failed to delete l3 interface for vlan: ",
        vlanMacAndIntf.first.first,
        " and mac : ",
        vlanMacAndIntf.first.second);
  }
  vlanAndMac2Intf_.clear();
}

void BcmWarmBootCache::removeUnclaimedStations() {
  for (auto vlanAndStation : vlan2Station_) {
    XLOG(DBG1) << "Deleting station for vlan : " << vlanAndStation.first;
    auto stationId =
        hw_->getPlatform()->getAsic()->getStationID(vlanAndStation.first);
    auto rv = bcm_l2_station_delete(hw_->getUnit(), stationId);
    bcmLogFatal(
        rv, hw_, "failed to delete station for vlan : ", vlanAndStation.first);
  }
  vlan2Station_.clear();
}

void BcmWarmBootCache::removeUnclaimedVlans() {
  bcm_vlan_t defaultVlan;
  auto rv = bcm_vlan_default_get(hw_->getUnit(), &defaultVlan);
  bcmLogFatal(rv, hw_, "failed to get default VLAN");
  // Finally delete the vlans
  for (auto vlanItr = vlan2VlanInfo_.begin();
       vlanItr != vlan2VlanInfo_.end();) {
    if (defaultVlan == vlanItr->first) {
      ++vlanItr;
      continue; // Can't delete the default vlan
    }
    XLOG(DBG1) << "Deleting vlan : " << vlanItr->first;
    auto rv = bcm_vlan_destroy(hw_->getUnit(), vlanItr->first);
    bcmLogFatal(rv, hw_, "failed to destroy vlan: ", vlanItr->first);
    vlanItr = vlan2VlanInfo_.erase(vlanItr);
  }
}

void BcmWarmBootCache::removeUnclaimedAclStats() {
  // Detach the unclaimed bcm acl stats
  flat_set<BcmAclStatHandle> statsUsed;
  for (auto aclStatItr = aclEntry2AclStat_.begin();
       aclStatItr != aclEntry2AclStat_.end();
       ++aclStatItr) {
    auto& aclStatStatus = aclStatItr->second;
    if (!aclStatStatus.claimed) {
      XLOG(DBG1) << "Detaching unclaimed acl_stat=" << aclStatStatus.stat
                 << "from acl=" << aclStatItr->first;
      BcmAclStat::detach(
          hw_,
          aclStatItr->first,
          aclStatStatus.stat,
          BcmAclStat::kDefaultAclActionIndex);
    } else {
      statsUsed.insert(aclStatStatus.stat);
    }
  }

  // Delete the unclaimed bcm acl stats
  for (auto statItr : aclEntry2AclStat_) {
    auto statHandle = statItr.second.stat;
    if (statsUsed.find(statHandle) == statsUsed.end()) {
      XLOG(DBG1) << "Deleting unclaimed acl_stat=" << statHandle;
      BcmAclStat::destroy(hw_, statHandle);
      // add the stat to the set to prevent this loop from attempting to
      // delete the same stat twice
      statsUsed.insert(statHandle);
    }
  }
  aclEntry2AclStat_.clear();
}

void BcmWarmBootCache::removeUnclaimedAcls() {
  // Delete acls, since acl(field process) doesn't support
  // bcm, we call BcmAclTable to remove the unclaimed acls
  XLOG(DBG1) << "Unclaimed acl count=" << priority2BcmAclEntryHandle_.size();
  for (auto aclItr : priority2BcmAclEntryHandle_) {
    XLOG(DBG1) << "Deleting unclaimed acl: prio=" << aclItr.first
               << ", handle=" << aclItr.second;
    removeBcmAcl(aclItr.second);
  }
  priority2BcmAclEntryHandle_.clear();
}

void BcmWarmBootCache::removeUnclaimedTeFlowStats() {
  // Detach the unclaimed bcm teflow stats
  std::set<BcmTeFlowStatHandle> teFlowStatsUsed;
  for (auto teFlowStatItr = teFlowEntry2TeFlowStat_.begin();
       teFlowStatItr != teFlowEntry2TeFlowStat_.end();
       ++teFlowStatItr) {
    auto& teFlowStatStatus = teFlowStatItr->second;
    if (!teFlowStatStatus.claimed) {
      XLOG(DBG1) << "Detaching unclaimed teflow_stat=" << teFlowStatStatus.stat
                 << "from teflow=" << teFlowStatItr->first;
      BcmTeFlowStat::detach(
          hw_,
          teFlowStatItr->first,
          teFlowStatStatus.stat,
          teFlowStatStatus.actionIndex);
    } else {
      teFlowStatsUsed.insert(teFlowStatStatus.stat);
    }
  }

  // Delete the unclaimed bcm teflow stats
  for (auto statItr : teFlowEntry2TeFlowStat_) {
    auto statHandle = statItr.second.stat;
    if (teFlowStatsUsed.find(statHandle) == teFlowStatsUsed.end()) {
      if (!hw_->getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
        XLOG(DBG1) << "Deleting unclaimed teflow_stat=" << statHandle;
        BcmTeFlowStat::destroy(hw_, statHandle);
        // add the stat to the set to prevent this loop from attempting to
        // delete the same stat twice
        teFlowStatsUsed.insert(statHandle);
      }
    }
  }
  teFlowEntry2TeFlowStat_.clear();
}

void BcmWarmBootCache::removeUnclaimedTeFlows() {
  XLOG(DBG1) << "Unclaimed teflow count="
             << teFlow2BcmTeFlowEntryHandle_.size();
  for (auto teFlowItr : teFlow2BcmTeFlowEntryHandle_) {
    XLOG(DBG1) << "Deleting unclaimed TeFlow: srcPort=" << teFlowItr.first.first
               << " dstIp=" << teFlowItr.first.second
               << ", handle=" << teFlowItr.second;
    removeBcmTeFlow(teFlowItr.second);
  }
  teFlow2BcmTeFlowEntryHandle_.clear();
}

void BcmWarmBootCache::removeUnclaimedCosqMappings() {
  int rv;
  bool useHSDK = (dynamic_cast<const BcmSwitch*>(hw_))->useHSDK();
  for (auto reasonToQueueEntry : index2ReasonToQueue_) {
    const auto index = reasonToQueueEntry.first;
    XLOG(DBG1) << "Deleting rx reason to queue entry: index=" << index
               << ", reason="
               << apache::thrift::util::enumNameSafe(
                      *reasonToQueueEntry.second.rxReason())
               << ", queue=" << *reasonToQueueEntry.second.queueId();
    if (!useHSDK) {
      rv = bcm_rx_cosq_mapping_delete(hw_->getUnit(), index);
    } else {
      bcm_rx_cosq_mapping_t cosqMap;
      bcm_rx_cosq_mapping_t_init(&cosqMap);
      cosqMap.index = index;
      rv = BcmControlPlane::rxCosqMappingExtendedDelete(
          hw_->getUnit(), &cosqMap);
    }
    bcmCheckError(rv, "failed to delete CPU cosq mapping for index ", index);
  }
  index2ReasonToQueue_.clear();
}

void BcmWarmBootCache::detachUdfPacketMatcher(
    const std::string& udfGroupName,
    bcm_udf_id_t udfId) {
  auto udfGroupPacketMatcherItr = findUdfGroupPacketMatcher(udfGroupName);
  if (udfGroupPacketMatcherItr != UdfGroupNameToPacketMatcherMapEnd()) {
    auto packetMatcherMap = udfGroupPacketMatcherItr->second;
    for (auto udfPktMatcherItr = packetMatcherMap.begin();
         udfPktMatcherItr != packetMatcherMap.end();
         udfPktMatcherItr++) {
      /* Detach packet matcher id from Udf Group */
      auto packetMatcherName = udfPktMatcherItr->first;
      auto packetMatcherId = udfPktMatcherItr->second;
      auto rv =
          bcm_udf_pkt_format_delete(hw_->getUnit(), udfId, packetMatcherId);
      bcmCheckError(
          rv,
          "Unable to detach packetMatcher: ",
          packetMatcherName,
          " packetMatcherId: ",
          packetMatcherId,
          " from udfGroup: ",
          udfGroupName,
          " udfGroupId: ",
          udfId);
      XLOG(DBG2) << "udfGroup=" << udfGroupName << " udfGroupId= " << udfId
                 << " packetMatcherName=" << packetMatcherName
                 << " packetMatcherId=" << packetMatcherId;
    }
  }
}

void BcmWarmBootCache::removeUnclaimedUdfGroups() {
  XLOG(DBG2) << "Unclaimed UdfGroups count=" << udfGroupNameToInfoMap_.size();
  for (auto udfGroupInfoItr = UdfGroupNameToInfoMapBegin();
       udfGroupInfoItr != UdfGroupNameToInfoMapEnd();
       udfGroupInfoItr++) {
    const auto& udfGroupName = udfGroupInfoItr->first;
    auto udfGroupId = udfGroupInfoItr->second.first;
    XLOG(DBG2) << "Deleting unclaimed UdfGroup: " << udfGroupName
               << " UdfGroupId: " << udfGroupId;
    detachUdfPacketMatcher(udfGroupName, udfGroupId);

    /* Delete udf */
    auto rv = bcm_udf_destroy(hw_->getUnit(), udfGroupId);
    bcmCheckError(
        rv,
        "Unable to delete  udfGroup: ",
        udfGroupName,
        " UdfGroupId: ",
        udfGroupId);
  }
  udfGroupNameToPacketMatcherMap_.clear();
  udfGroupNameToInfoMap_.clear();
}

void BcmWarmBootCache::removeUnclaimedUdfPacketMatchers() {
  XLOG(DBG2) << "Unclaimed UdfPacketMatchers count="
             << udfPktMatcherNameToInfoMap_.size();
  for (auto udfPktMatcherItr = UdfPktMatcherNameToInfoMapBegin();
       udfPktMatcherItr != UdfPktMatcherNameToInfoMapEnd();
       udfPktMatcherItr++) {
    const auto& udfPktMatcherName = udfPktMatcherItr->first;
    auto udfPktMatcherId = udfPktMatcherItr->second.first;
    XLOG(DBG2) << "Deleting unclaimed udfPktMatcher: " << udfPktMatcherName
               << " udfPktMatcherId:" << udfPktMatcherId;
    /* Delete the packet format */
    auto rv = bcm_udf_pkt_format_destroy(hw_->getUnit(), udfPktMatcherId);
    bcmCheckError(
        rv,
        "Unable to delete  udfPktMatcher: ",
        udfPktMatcherName,
        " udfPktMatcherId:",
        udfPktMatcherId);
  }
  udfPktMatcherNameToInfoMap_.clear();
}

void BcmWarmBootCache::clear() {
  // Get rid of all unclaimed entries. The order is important here
  // since we want to delete entries only after there are no more
  // references to them.
  XLOG(DBG1) << "Warm boot: removing unreferenced entries";
  dumpedSwSwitchState_.reset();
  hwSwitchEcmp2EgressIds_.clear();
  // First delete routes (fully qualified and others).
  //
  // Nothing references routes, but routes reference ecmp egress and egress
  // entries which are deleted later
  removeUnclaimedRoutes();

  // purge any lingering label FIB entries
  removeUnclaimedLabelSwitchActions();

  // Delete bcm host entries. Nobody references bcm hosts, but
  // hosts reference egress objects
  removeUnclaimedHostEntries();

  // Both routes and host entries (which have been deleted earlier) can refer
  // to ecmp egress objects.  Ecmp egress objects in turn refer to egress
  // objects which we delete later
  removeUnclaimedEcmpEgressObjects();

  // Delete bcm egress entries. These are referenced by routes, ecmp egress
  // and host objects all of which we deleted above. Egress objects in turn
  // my point to a interface which we delete later
  removeUnclaimedEgressObjects();

  // delete any MPLS tunnels
  removeUnclaimedLabeledTunnels();

  // Delete interfaces
  removeUnclaimedInterfaces();
  // Delete stations
  removeUnclaimedStations();
  // Finally delete the vlans
  removeUnclaimedVlans();

  egressId2WeightInWarmBootFile_.clear();
  vrfIp2EgressFromBcmHostInWarmBootFile_.clear();

  // Delete the unclaimed bcm acl stats
  removeUnclaimedAclStats();

  // Delete acls
  removeUnclaimedAcls();

  // Delete the unclaimed bcm teflow stats
  removeUnclaimedTeFlowStats();

  // Delete TeFlows.
  removeUnclaimedTeFlows();

  // Delete CosQMappings
  removeUnclaimedCosqMappings();

  // Delete UdfGroups
  removeUnclaimedUdfGroups();

  // Delete UdfPacketMatchers
  removeUnclaimedUdfPacketMatchers();

  /* remove unclaimed mirrors and mirrored ports/acls, if any */
  checkUnclaimedMirrors();
  checkUnclaimedQosMaps();

  ptpTcEnabled_ = std::nullopt;
  // Clear TeFlow
  teFlowDstIpPrefixLength_ = 0;
  teFlowHintId_ = 0;
  teFlowGroupId_ = 0;
  teFlowFlexCounterId_ = 0;
}

void BcmWarmBootCache::populateRtag7State() {
  auto unit = hw_->getUnit();

  moduleAState_ = BcmRtag7Module::retrieveRtag7ModuleState(
      unit, BcmRtag7Module::kModuleAControl());
  moduleBState_ = BcmRtag7Module::retrieveRtag7ModuleState(
      unit, BcmRtag7Module::kModuleBControl());
  ecmpOutputSelectionState_ = BcmRtag7Module::retrieveRtag7OutputState(
      unit, BcmRtag7Module::kEcmpOutputSelectionControl());
  trunkOutputSelectionState_ = BcmRtag7Module::retrieveRtag7OutputState(
      unit,
      BcmRtag7Module::kTrunkOutputSelectionControl(
          hw_->getPlatform()->getAsic()));
}

bool BcmWarmBootCache::unitControlMatches(
    char module,
    bcm_switch_control_t switchControl,
    int arg) const {
  const BcmRtag7Module::ModuleState* state = nullptr;

  switch (module) {
    case 'A':
      state = &moduleAState_;
      break;
    case 'B':
      state = &moduleBState_;
      break;
    default:
      throw FbossError("Invalid module identifier ", module);
  }

  CHECK(state);
  auto it = state->find(switchControl);

  return it != state->end() && it->second == arg;
}

void BcmWarmBootCache::programmed(
    char module,
    bcm_switch_control_t switchControl) {
  BcmRtag7Module::ModuleState* state = nullptr;
  switch (module) {
    case 'A':
      state = &moduleAState_;
      break;
    case 'B':
      state = &moduleBState_;
      break;
    default:
      throw FbossError("Invalid module identifier ", module);
  }

  CHECK(state);
  auto numErased = state->erase(switchControl);

  CHECK_EQ(numErased, 1);
}

bool BcmWarmBootCache::unitControlMatches(
    LoadBalancerID loadBalancerID,
    int switchControl,
    int arg) const {
  const BcmRtag7Module::OutputSelectionState* state = nullptr;

  switch (loadBalancerID) {
    case cfg::LoadBalancerID::ECMP:
      state = &ecmpOutputSelectionState_;
      break;
    case cfg::LoadBalancerID::AGGREGATE_PORT:
      state = &trunkOutputSelectionState_;
      break;
  }

  CHECK(state);
  auto it = state->find(switchControl);

  return it != state->end() && it->second == arg;
}

void BcmWarmBootCache::programmed(
    LoadBalancerID loadBalancerID,
    int switchControl) {
  BcmRtag7Module::OutputSelectionState* state = nullptr;

  switch (loadBalancerID) {
    case cfg::LoadBalancerID::ECMP:
      state = &ecmpOutputSelectionState_;
      break;
    case cfg::LoadBalancerID::AGGREGATE_PORT:
      state = &trunkOutputSelectionState_;
      break;
  }

  CHECK(state);
  auto numErased = state->erase(switchControl);

  CHECK_EQ(numErased, 1);
}

BcmWarmBootCache::MirrorEgressPath2HandleCitr BcmWarmBootCache::findMirror(
    bcm_gport_t port,
    const std::optional<MirrorTunnel>& tunnel) const {
  return mirrorEgressPath2Handle_.find(std::make_pair(port, tunnel));
}

BcmWarmBootCache::MirrorEgressPath2HandleCitr BcmWarmBootCache::mirrorsBegin()
    const {
  return mirrorEgressPath2Handle_.begin();
}
BcmWarmBootCache::MirrorEgressPath2HandleCitr BcmWarmBootCache::mirrorsEnd()
    const {
  return mirrorEgressPath2Handle_.end();
}

void BcmWarmBootCache::programmedMirror(MirrorEgressPath2HandleCitr itr) {
  const auto key = itr->first;
  const auto port = key.first;
  const auto& tunnel = key.second;
  if (tunnel) {
    XLOG(DBG1) << "Programmed ERSPAN mirror egressing through: " << port
               << " with "
               << "proto=" << tunnel->greProtocol
               << "source ip=" << tunnel->srcIp.str()
               << "source mac=" << tunnel->srcMac.toString()
               << "destination ip=" << tunnel->dstIp.str()
               << "destination mac=" << tunnel->dstMac.toString()
               << ", removing from warm boot cache";
  } else {
    XLOG(DBG1) << "Programmed SPAN mirror egressing through: " << port
               << ", removing from warm boot cache";
  }
  mirrorEgressPath2Handle_.erase(itr);
}

BcmWarmBootCache::MirroredPort2HandleCitr BcmWarmBootCache::mirroredPortsBegin()
    const {
  return mirroredPort2Handle_.begin();
}

BcmWarmBootCache::MirroredPort2HandleCitr BcmWarmBootCache::mirroredPortsEnd()
    const {
  return mirroredPort2Handle_.end();
}

BcmWarmBootCache::MirroredPort2HandleCitr BcmWarmBootCache::findMirroredPort(
    bcm_gport_t port,
    uint32_t flags) const {
  return mirroredPort2Handle_.find(std::make_pair(port, flags));
}

void BcmWarmBootCache::programmedMirroredPort(MirroredPort2HandleCitr itr) {
  auto handle = itr->second;
  auto flags = itr->first.second;
  if (isSflowMirror(handle)) {
    /* if sflow mirror is claimed by first port, then claim it for all ports */
    auto currItr = mirroredPort2Handle_.cbegin();
    while (currItr != mirroredPort2Handle_.cend()) {
      if (currItr->first.second == flags && currItr->second == handle) {
        currItr = mirroredPort2Handle_.erase(currItr);
      } else {
        currItr++;
      }
    }
  } else {
    mirroredPort2Handle_.erase(itr);
  }
}

BcmWarmBootCache::MirroredAcl2HandleCitr BcmWarmBootCache::mirroredAclsBegin()
    const {
  return mirroredAcl2Handle_.begin();
}

BcmWarmBootCache::MirroredAcl2HandleCitr BcmWarmBootCache::mirroredAclsEnd()
    const {
  return mirroredAcl2Handle_.end();
}

BcmWarmBootCache::MirroredAcl2HandleCitr BcmWarmBootCache::findMirroredAcl(
    BcmAclEntryHandle entry,
    MirrorDirection direction) const {
  return mirroredAcl2Handle_.find(std::make_pair(entry, direction));
}

void BcmWarmBootCache::programmedMirroredAcl(MirroredAcl2HandleCitr itr) {
  mirroredAcl2Handle_.erase(itr);
}

void BcmWarmBootCache::checkUnclaimedMirrors() {
  /* no spurious entries should exist around mirrors */
  CHECK_EQ(mirroredPort2Handle_.size(), 0)
      << "Unclaimed mirrored port count=" << mirroredPort2Handle_.size();

  CHECK_EQ(mirroredAcl2Handle_.size(), 0)
      << "Unclaimed mirrored acl count=" << mirroredAcl2Handle_.size();

  CHECK_EQ(mirrorEgressPath2Handle_.size(), 0)
      << "unclaimed mirror count count=" << mirrorEgressPath2Handle_.size();
}

void BcmWarmBootCache::programmed(TrunksItr itr) {
  XLOG(DBG1) << "Programmed trunk id=" << itr->second
             << ", removing from warm boot cache.";
  trunks_.erase(itr);
}

void BcmWarmBootCache::checkUnclaimedQosMaps() {
  if (qosMapKey2QosMapId_.size()) {
    XLOG(ERR) << "Unclaimed qosMapKey2QosMapId_ entries, count ="
              << qosMapKey2QosMapId_.size() << " [ ";
    for (const auto& qosMapKey2Qos : qosMapKey2QosMapId_) {
      const auto& qosMapKey2QosPairKey = qosMapKey2Qos.first;
      XLOG(ERR) << " Policy name: " << qosMapKey2QosPairKey.first
                << ", type: " << (int)qosMapKey2QosPairKey.second;
    }
    XLOG(ERR) << "]";
  }
  CHECK_EQ(qosMapKey2QosMapId_.size() + qosMapId2QosMap_.size(), 0)
      << "unclaimed qos map entries found. qosMapId2QosMap_ count ="
      << qosMapId2QosMap_.size();
}

BcmWarmBootCache::QosMapId2QosMapItr BcmWarmBootCache::findQosMap(
    const std::shared_ptr<QosPolicy>& qosPolicy,
    BcmQosMap::Type type) {
  /* for a given policy find if qos map is in saved hw switch state */
  auto itr =
      qosMapKey2QosMapId_.find(std::make_pair(qosPolicy->getName(), type));
  if (itr == qosMapKey2QosMapId_.end()) {
    return qosMapId2QosMap_.end();
  }
  /* for a given policy, if qos map is in saved hw switch state then it must be
   * in warm boot cache */
  auto qosMapId2QosMapItr = qosMapId2QosMap_.find(itr->second);
  CHECK(qosMapId2QosMapItr != qosMapId2QosMap_.end())
      << "qos map id " << itr->second << " not found";

  /* collect rules that must be in qos map */
  std::set<std::pair<uint16_t, uint8_t>> mapEntries;
  switch (type) {
    case BcmQosMap::Type::MPLS_INGRESS:
      for (const auto& entry : std::as_const(
               *(qosPolicy->getExpMap()->cref<switch_state_tags::from>()))) {
        auto tc = entry->get<switch_state_tags::trafficClass>()->toThrift();
        auto attr = entry->get<switch_state_tags::attr>()->toThrift();
        mapEntries.emplace(tc, attr);
      }
      break;
    case BcmQosMap::Type::MPLS_EGRESS:
      for (const auto& entry : std::as_const(
               *(qosPolicy->getExpMap()->cref<switch_state_tags::to>()))) {
        auto tc = entry->get<switch_state_tags::trafficClass>()->toThrift();
        auto attr = entry->get<switch_state_tags::attr>()->toThrift();
        mapEntries.emplace(tc, attr);
      }
      break;
    case BcmQosMap::Type::IP_INGRESS:
      for (const auto& entry : std::as_const(
               *(qosPolicy->getDscpMap()->cref<switch_state_tags::from>()))) {
        auto tc = entry->get<switch_state_tags::trafficClass>()->toThrift();
        auto attr = entry->get<switch_state_tags::attr>()->toThrift();
        mapEntries.emplace(tc, attr);
      }
      break;
    case BcmQosMap::Type::IP_EGRESS:
      XLOG(FATAL) << "L3 egress qos map is not supported";
  }

  for (const auto& entry : mapEntries) {
    if (!qosMapId2QosMapItr->second->ruleExists(entry.first, entry.second)) {
      /* if any rule doesn't exist, then avoid claiming it */
      return qosMapId2QosMap_.end();
    }
  }

  if (qosMapId2QosMapItr->second->size() != mapEntries.size()) {
    /* sw switch qos policy rules are only subset of  rules in qos map */
    XLOG(WARN) << " Mismatch in QosMap size of type: "
               << qosMapId2QosMapItr->second->getType()
               << " with id: " << qosMapId2QosMapItr->second->getHandle()
               << " for policy: " << qosPolicy->getName()
               << " Expected size: " << mapEntries.size()
               << ", HW entry size: " << qosMapId2QosMapItr->second->size();
    return qosMapId2QosMap_.end();
  }

  XLOG(DBG1) << "Found QosMap of type " << qosMapId2QosMapItr->second->getType()
             << " with id " << qosMapId2QosMapItr->second->getHandle()
             << " for policy " << qosPolicy->getName()
             << ", removing from warm boot cache.";
  return qosMapId2QosMapItr;
}

void BcmWarmBootCache::programmed(
    const std::string& policyName,
    BcmQosMap::Type type,
    BcmWarmBootCache::QosMapId2QosMapItr itr) {
  qosMapKey2QosMapId_.erase(std::make_pair(policyName, type));
  qosMapId2QosMap_.erase(itr);
}

namespace {
std::optional<MirrorTunnel> getMirrorTunnel(
    const bcm_mirror_destination_t* mirror_dest) {
  std::optional<TunnelUdpPorts> udpPorts;
  if (!(mirror_dest->flags &
        (BCM_MIRROR_DEST_TUNNEL_SFLOW | BCM_MIRROR_DEST_TUNNEL_IP_GRE))) {
    return std::nullopt;
  }
  if (mirror_dest->flags & BCM_MIRROR_DEST_TUNNEL_SFLOW) {
    udpPorts =
        TunnelUdpPorts(mirror_dest->udp_src_port, mirror_dest->udp_dst_port);
  }
  if (mirror_dest->version == 4) {
    return udpPorts ? MirrorTunnel(
                          folly::IPAddress::fromLongHBO(mirror_dest->src_addr),
                          folly::IPAddress::fromLongHBO(mirror_dest->dst_addr),
                          macFromBcm(mirror_dest->src_mac),
                          macFromBcm(mirror_dest->dst_mac),
                          udpPorts.value(),
                          mirror_dest->ttl)
                    : MirrorTunnel(
                          folly::IPAddress::fromLongHBO(mirror_dest->src_addr),
                          folly::IPAddress::fromLongHBO(mirror_dest->dst_addr),
                          macFromBcm(mirror_dest->src_mac),
                          macFromBcm(mirror_dest->dst_mac),
                          mirror_dest->ttl);
  } else {
    return udpPorts ? MirrorTunnel(
                          folly::IPAddress::fromBinary(
                              folly::ByteRange(mirror_dest->src6_addr, 16)),
                          folly::IPAddress::fromBinary(
                              folly::ByteRange(mirror_dest->dst6_addr, 16)),
                          macFromBcm(mirror_dest->src_mac),
                          macFromBcm(mirror_dest->dst_mac),
                          udpPorts.value(),
                          mirror_dest->ttl)
                    : MirrorTunnel(
                          folly::IPAddress::fromBinary(
                              folly::ByteRange(mirror_dest->src6_addr, 16)),
                          folly::IPAddress::fromBinary(
                              folly::ByteRange(mirror_dest->dst6_addr, 16)),
                          macFromBcm(mirror_dest->src_mac),
                          macFromBcm(mirror_dest->dst_mac),
                          mirror_dest->ttl);
  }
}

std::string tunnelInitiatorString(
    bcm_if_t intf,
    bcm_vlan_t vid,
    const LabelForwardingAction::LabelStack& labels) {
  return folly::to<std::string>(
      "mpls tunnel(",
      intf,
      "/",
      vid,
      ")",
      folly::to<std::string>("@stack[", folly::join(",", labels), "]"));
}
} // namespace
void BcmWarmBootCache::populateAcls(
    const int groupId,
    AclEntry2AclStat& stats,
    Priority2BcmAclEntryHandle& acls) {
  int entryCount = 0;
  // first get the count of field entries of this group
  auto rv = bcm_field_entry_multi_get(
      hw_->getUnit(), groupId, 0, nullptr, &entryCount);
  bcmCheckError(rv, "Unable to get count of field entry for group: ", groupId);
  XLOG(DBG1) << "Existing entry count=" << entryCount
             << " for group=" << groupId;

  if (!entryCount) {
    return;
  }
  std::vector<bcm_field_entry_t> bcmEntries(entryCount);
  rv = bcm_field_entry_multi_get(
      hw_->getUnit(), groupId, entryCount, bcmEntries.data(), &entryCount);
  bcmCheckError(
      rv, "Unable to get field entry information for group=", groupId);
  for (auto bcmEntry : bcmEntries) {
    // Get acl stat associated to each acl entry
    populateAclStats(groupId, bcmEntry, stats);
    // Get priority
    int priority = 0;
    rv = bcm_field_entry_prio_get(hw_->getUnit(), bcmEntry, &priority);
    bcmCheckError(rv, "Unable to get priority for entry=", bcmEntry);
    // Right now we don't support to have the same priority for two acls.
    CHECK(acls.find(priority) == acls.end());
    // convert the prio back to s/w priority
    acls.emplace(utility::swPriorityToHwPriority(priority), bcmEntry);

    populateMirroredAcl(bcmEntry);
  }
}

void BcmWarmBootCache::populateAclStats(
    int groupID,
    BcmAclEntryHandle aclHandle,
    AclEntry2AclStat& stats) {
  auto aclStatHandle =
      BcmAclStat::getAclStatHandleFromAttachedAcl(hw_, groupID, aclHandle);
  if (!aclStatHandle) {
    // This acl doesn't have any BcmAclStat attached
    return;
  }
  AclStatStatus statStatus;
  statStatus.stat = (*aclStatHandle).first;
  stats.emplace(aclHandle, statStatus);
}

void BcmWarmBootCache::removeBcmAcl(BcmAclEntryHandle handle) {
  auto rv = bcm_field_entry_destroy(hw_->getUnit(), handle);
  bcmLogFatal(rv, hw_, "failed to destroy the acl entry");
}

void BcmWarmBootCache::populateTeFlows(
    const int groupId,
    TeFlowEntry2TeFlowStat& stats,
    TeFlow2BcmTeFlowEntryHandle& teflows) {
  int entryCount = 0;
  // first get the count of EM entries of this group
  auto rv = bcm_field_entry_multi_get(
      hw_->getUnit(), getEMGroupID(groupId), 0, nullptr, &entryCount);
  if (rv == BCM_E_NOT_FOUND) {
    XLOG(DBG1) << "EM group does not exist";
    return;
  }
  bcmCheckError(rv, "Unable to get count of EM entry for group: ", groupId);
  XLOG(DBG1) << "Existing EM entry count=" << entryCount
             << " for group=" << groupId;

  if (!entryCount) {
    return;
  }
  std::vector<bcm_field_entry_t> bcmEntries(entryCount);
  rv = bcm_field_entry_multi_get(
      hw_->getUnit(),
      getEMGroupID(groupId),
      entryCount,
      bcmEntries.data(),
      &entryCount);
  bcmCheckError(rv, "Unable to get EM entry information for group=", groupId);
  for (auto bcmEntry : bcmEntries) {
    // Get teflow stat associated to each teflow entry
    populateTeFlowStats(groupId, bcmEntry, stats);
    // Get srcPort of teflow  entry
    bcm_module_t data_modid;
    bcm_module_t mask_modid;
    bcm_port_t hwSrcPort;
    bcm_port_t mask;
    rv = bcm_field_qualify_SrcPort_get(
        hw_->getUnit(), bcmEntry, &data_modid, &mask_modid, &hwSrcPort, &mask);
    bcmCheckError(rv, "Unable to get src port for entry=", bcmEntry);

    // Get destIp of teflow  entry
    bcm_ip6_t hwAddr{};
    bcm_ip6_t hwMask{};
    rv = bcm_field_qualify_DstIp6_get(
        hw_->getUnit(), bcmEntry, &hwAddr, &hwMask);
    bcmCheckError(rv, "Unable to get Dst Ip6 for entry=", bcmEntry);

    auto destIp = ipFromBcm(hwAddr);
    XLOG(DBG2) << "Populate TeFlowEntry, recovered from h/w. "
               << "Teflow: srcPort=" << hwSrcPort << " destIp= " << destIp
               << " teflow entry=" << bcmEntry;
    CHECK(teflows.find({hwSrcPort, destIp}) == teflows.end());
    teflows.emplace(std::make_pair(hwSrcPort, destIp), bcmEntry);
  }

  XLOG(DBG1) << "Teflow map count=" << teflows.size()
             << " for group=" << groupId;
  CHECK_EQ(teflows.size(), entryCount);
}

void BcmWarmBootCache::populateTeFlowStats(
    int groupId,
    BcmTeFlowEntryHandle teFlowHandle,
    TeFlowEntry2TeFlowStat& stats) {
  auto teFlowStatHandle = BcmTeFlowStat::getAclStatHandleFromAttachedAcl(
      hw_, getEMGroupID(groupId), teFlowHandle);
  if (!teFlowStatHandle) {
    // This teflow doesn't have any BcmTeFlowStat attached
    return;
  }
  TeFlowStatStatus statStatus;
  statStatus.stat = (*teFlowStatHandle).first;
  statStatus.actionIndex = (*teFlowStatHandle).second;
  stats.emplace(teFlowHandle, statStatus);
}

void BcmWarmBootCache::removeBcmTeFlow(BcmTeFlowEntryHandle handle) {
  auto rv = bcm_field_entry_destroy(hw_->getUnit(), handle);
  bcmLogFatal(rv, hw_, "failed to destroy the teflow entry");
}

void BcmWarmBootCache::populateRxReasonToQueue() {
  const int maxCPUMappings = hw_->getControlPlane()->getMaxRxReasonMappings();
  for (int i = 0; i < maxCPUMappings; i++) {
    if (const auto entry = hw_->getControlPlane()->getReasonToQueueEntry(i)) {
      index2ReasonToQueue_[i] = *entry;
    }
  }
}

void BcmWarmBootCache::populateMirrors() {
  auto saveMirrorDescriptor = [&](bcm_mirror_destination_t* mirror_dest) {
    /* save in map the mirror descriptor matching properties of input mirror */
    auto bcmEgressPort = mirror_dest->gport;
    auto bcmMirrorTunnel = getMirrorTunnel(mirror_dest);
    mirrorEgressPath2Handle_.emplace(
        std::make_pair(bcmEgressPort, bcmMirrorTunnel),
        mirror_dest->mirror_dest_id);
  };

  auto mirrorTraverseCb =
      [](int /*unit*/, bcm_mirror_destination_t* mirror_dest, void* saver) {
        (*static_cast<decltype(saveMirrorDescriptor)*>(saver))(mirror_dest);
        return 0;
      };

  /* traverse all mirrors and save mirror descriptors
   */
  auto rv = bcm_mirror_destination_traverse(
      hw_->getUnit(), mirrorTraverseCb, &saveMirrorDescriptor);
  bcmCheckError(rv, "Failed to traverse mirrors");
}

void BcmWarmBootCache::populateMirroredPorts() {
  bcm_port_config_t config;
  bcm_port_config_t_init(&config);
  bcm_port_config_get(hw_->getUnit(), &config);
  bcm_port_t port;
  bcm_gport_t gport;
  BCM_PBMP_ITER(config.port, port) {
    BCM_GPORT_MODPORT_SET(gport, hw_->getUnit(), port);
    populateMirroredPort(gport);
  }
}

void BcmWarmBootCache::populateMirroredPort(bcm_gport_t port) {
  std::vector<MirrorDirection> directions{
      MirrorDirection::INGRESS, MirrorDirection::EGRESS};
  std::vector<cfg::SampleDestination> destinations{
      cfg::SampleDestination::CPU, cfg::SampleDestination::MIRROR};

  for (auto direction : directions) {
    for (auto destination : destinations) {
      // sampling to mirrors is not valid 1. with egress samples and 2. if sflow
      // isn't supported
      if (destination == cfg::SampleDestination::MIRROR &&
          (direction == MirrorDirection::EGRESS ||
           !getHw()->getPlatform()->getAsic()->isSupported(
               HwAsic::Feature::SFLOW_SAMPLING))) {
        continue;
      }
      bcm_gport_t mirror_dest = 0;
      int mirror_dest_count = 0;
      uint32_t flag = directionToBcmPortMirrorFlag(direction) |
          sampleDestinationToBcmPortMirrorSflowFlag(destination);
      auto rv = bcm_mirror_port_dest_get(
          hw_->getUnit(), port, flag, 1, &mirror_dest, &mirror_dest_count);
      bcmCheckError(rv, "Failed to get mirror port destination");
      CHECK_LE(mirror_dest_count, 1);
      if (mirror_dest_count) {
        mirroredPort2Handle_.emplace(std::make_pair(port, flag), mirror_dest);
      }
    }
  }
}

void BcmWarmBootCache::populateMirroredAcl(BcmAclEntryHandle entry) {
  std::vector<MirrorDirection> directions{
      MirrorDirection::INGRESS, MirrorDirection::EGRESS};

  for (auto direction : directions) {
    uint32_t param0 = 0;
    uint32_t param1 = 0;
    bcm_field_action_get(
        hw_->getUnit(),
        entry,
        directionToBcmAclMirrorAction(direction),
        &param0,
        &param1);
    if (param1) {
      mirroredAcl2Handle_.emplace(std::make_pair(entry, direction), param1);
    }
  }
}

void BcmWarmBootCache::populateQosMaps() {
  constexpr auto kQosMapIngressL3Flags = BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3;
  constexpr auto kQosMapIngressMplsFlags =
      BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_MPLS;
  constexpr auto kQosMapEgressMplslags = BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_MPLS;

  auto mapIdsAndFlags = getBcmQosMapIdsAndFlags(hw_->getUnit());
  for (auto mapIdAndFlag : mapIdsAndFlags) {
    int id = std::get<0>(mapIdAndFlag);
    int flags = std::get<1>(mapIdAndFlag);
    if ((flags & kQosMapIngressL3Flags) == kQosMapIngressL3Flags) {
      qosMapId2QosMap_.emplace(id, std::make_unique<BcmQosMap>(hw_, flags, id));
    } else if ((flags & kQosMapIngressMplsFlags) == kQosMapIngressMplsFlags) {
      qosMapId2QosMap_.emplace(id, std::make_unique<BcmQosMap>(hw_, flags, id));
    } else if ((flags & kQosMapEgressMplslags) == kQosMapEgressMplslags) {
      qosMapId2QosMap_.emplace(id, std::make_unique<BcmQosMap>(hw_, flags, id));
    } else {
      XLOG(WARNING) << "unknown qos map " << id << "discovered with flags "
                    << flags;
    }
  }
}

void BcmWarmBootCache::populateLabelSwitchActions() {
  auto saveLabelSwitchAction = [this](bcm_mpls_tunnel_switch_t* info) {
    auto bcmMplsTunnelSwitch = std::make_unique<BcmMplsTunnelSwitchT>();
    bcmMplsTunnelSwitch->get()->data = *info;
    label2LabelActions_.emplace(info->label, std::move(bcmMplsTunnelSwitch));
  };

  auto mplsTunnelSwitchTraverse =
      [](int /*unit*/, bcm_mpls_tunnel_switch_t* info, void* user_data) -> int {
    (*static_cast<decltype(saveLabelSwitchAction)*>(user_data))(info);
    return 0;
  };

  auto rv = bcm_mpls_tunnel_switch_traverse(
      hw_->getUnit(), mplsTunnelSwitchTraverse, &saveLabelSwitchAction);
  bcmCheckError(rv, "Failed to traverse label switch actions");
}

void BcmWarmBootCache::removeUnclaimedLabelSwitchActions() {
  for (const auto& entries : label2LabelActions_) {
    auto& bcmMplsTunnelSwitch = entries.second;
    auto& info = bcmMplsTunnelSwitch->get()->data;
    auto rv = bcm_mpls_tunnel_switch_delete(hw_->getUnit(), &info);
    bcmCheckError(
        rv,
        "failed to remove unclaimed label switch action for label:",
        info.label);
  }
  label2LabelActions_.clear();
}

void BcmWarmBootCache::populateLabelStack2TunnelId(bcm_l3_egress_t* egress) {
  CHECK(egress);
  auto* bcm_egress = reinterpret_cast<bcm_l3_egress_t*>(egress);
  if (bcm_egress->mpls_label == BCM_MPLS_LABEL_INVALID) {
    return;
  }
  int label_count = 0;
  std::vector<bcm_mpls_egress_label_t> egress_labels{
      hw_->getPlatform()->getAsic()->getMaxLabelStackDepth()};

  // TODO(pshaikh): case to open as bcm_mpls_tunnel_initiator_get doesn't work
  // if 3rd arg is 0 and 4th arg is nullptr. ideally, we would want that to
  // get count of labels and then reset labels vector accordingly
  auto rv = bcm_mpls_tunnel_initiator_get(
      hw_->getUnit(),
      bcm_egress->intf,
      hw_->getPlatform()->getAsic()->getMaxLabelStackDepth(),
      egress_labels.data(),
      &label_count);
  if (rv == BCM_E_NOT_FOUND) {
    // not an MPLS tunnel
    return;
  }
  egress_labels.clear();
  bcm_l3_intf_t intf;
  bcm_l3_intf_t_init(&intf);
  intf.l3a_intf_id = bcm_egress->intf;
  intf.l3a_flags = BCM_L3_WITH_ID;
  rv = bcm_l3_intf_get(hw_->getUnit(), &intf);

  if (label_count) {
    egress_labels.resize(label_count);
    bcm_mpls_tunnel_initiator_get(
        hw_->getUnit(),
        intf.l3a_intf_id,
        label_count,
        egress_labels.data(),
        &label_count);
  }
  LabelForwardingAction::LabelStack labels;

  std::transform(
      std::begin(egress_labels),
      std::end(egress_labels),
      std::back_inserter(labels),
      [](const bcm_mpls_egress_label_t& egress_label) {
        return egress_label.label;
      });

  auto key = BcmWarmBootCache::LabelStackKey(intf.l3a_vid, std::move(labels));
  if (labelStackKey2TunnelId_.find(key) != labelStackKey2TunnelId_.end()) {
    return;
  }

  XLOG(DBG4) << "found "
             << tunnelInitiatorString(
                    intf.l3a_intf_id, intf.l3a_vid, key.second);
  labelStackKey2TunnelId_.emplace(std::move(key), intf.l3a_intf_id);
}

void BcmWarmBootCache::removeUnclaimedLabeledTunnels() {
  auto iter = std::begin(labelStackKey2TunnelId_);

  while (iter != std::end(labelStackKey2TunnelId_)) {
    auto vid = iter->first.first;
    const auto& labels = iter->first.second;
    auto intfId = iter->second;
    auto name = tunnelInitiatorString(intfId, vid, labels);
    auto rv = bcm_mpls_tunnel_initiator_clear(hw_->getUnit(), intfId);
    bcmCheckError(rv, "failed to clear ", name);
    XLOG(DBG3) << "cleared " << name;
    bcm_l3_intf_t intf;
    bcm_l3_intf_t_init(&intf);
    intf.l3a_intf_id = intfId;
    intf.l3a_flags = BCM_L3_WITH_ID;
    rv = bcm_l3_intf_delete(hw_->getUnit(), &intf);
    bcmCheckError(rv, "failed to delete ", name);
    iter = labelStackKey2TunnelId_.erase(iter);
  }
}

bool BcmWarmBootCache::isSflowMirror(BcmMirrorHandle handle) const {
  bcm_mirror_destination_t mirror_dest;
  bcm_mirror_destination_t_init(&mirror_dest);
  auto rv = bcm_mirror_destination_get(hw_->getUnit(), handle, &mirror_dest);
  bcmCheckError(rv, "failed to get mirror port:", handle);
  if (mirror_dest.flags & BCM_MIRROR_DEST_TUNNEL_SFLOW) {
    return true;
  }
  return false;
}

void BcmWarmBootCache::populateSwitchSettings() {
  uint32_t flags = 0;

  bcm_port_config_t config;
  bcm_port_config_t_init(&config);
  bcm_port_config_get(hw_->getUnit(), &config);
  bcm_port_t port;

  BCM_PBMP_ITER(config.port, port) {
    uint32_t port_flags;
    auto rv = bcm_port_learn_get(hw_->getUnit(), port, &port_flags);
    bcmCheckError(rv, "Unable to get L2 Learning flags for port: ", port);

    if (flags == 0) {
      flags = port_flags;
    } else if (flags != port_flags) {
      throw FbossError("Every port should have same L2 Learning setting");
    }
  }

  // This is warm boot, so there cannot be any L2 update callback registered.
  // Thus, BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_FWD should be enough to
  // ascertain HARDWARE as L2 learning mode.
  // NOTE:
  //   This logic to determine the configured L2 learning mode
  //   in SDK/ASIC works only for TH as it's the only ASIC that
  //   uses L2 learn pending bit.
  //   So, this warmboot cache state shouldn't be used in making the
  //   decision to whether call the L2 learning mode config API during
  //   warmboot or not.
  //   TODO: Check with Broadcom on how to correctly identify the SDK/HW
  //   config for this that would work for all ASIC platforms.
  if (flags == (BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_FWD)) {
    l2LearningMode_ = cfg::L2LearningMode::HARDWARE;
  } else if (flags == (BCM_PORT_LEARN_ARL | BCM_PORT_LEARN_PENDING)) {
    l2LearningMode_ = cfg::L2LearningMode::SOFTWARE;
  } else {
    throw FbossError(
        "L2 Learning mode is neither SOFTWARE, nor HARDWARE, flags: ", flags);
  }

  XLOG(DBG3) << "Check if PTP TC is enabled to populate warmboot cache";
  ptpTcEnabled_ = BcmPtpTcMgr::isPtpTcEnabled(hw_);
}

} // namespace facebook::fboss
