/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmWarmBootCache.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <utility>

#include <folly/Conv.h>
#include <folly/dynamic.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>

#include "fboss/agent/Constants.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmEgress.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmMirrorTable.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmPortQueueManager.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/InterfaceMap.h"
#include "fboss/agent/state/LoadBalancer.h"
#include "fboss/agent/state/LoadBalancerMap.h"
#include "fboss/agent/state/MirrorMap.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

extern "C" {
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

namespace {
auto constexpr kEcmpObjects = "ecmpObjects";
auto constexpr kTrunks = "trunks";
auto constexpr kVlanForCPUEgressEntries = 0;
auto constexpr kACLFieldGroupID = 128;

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

  return bcmWarmBootState;
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

const BcmWarmBootCache::EgressIds& BcmWarmBootCache::getPathsForEcmp(
    EgressId ecmp) const {
  static const EgressIds kEmptyEgressIds;
  if (hwSwitchEcmp2EgressIds_.empty()) {
    // We may have empty hwSwitchEcmp2EgressIds_ when
    // we exited with no ECMP entries.
    return kEmptyEgressIds;
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
      paths.push_back(path);
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
  return warmBootCache;
}

folly::dynamic BcmWarmBootCache::getWarmBootState() const {
  return hw_->getPlatform()->getWarmBootHelper()->getWarmBootState();
}

void BcmWarmBootCache::populateFromWarmBootState(
    const folly::dynamic& warmBootState) {
  dumpedSwSwitchState_ =
      SwitchState::uniquePtrFromFollyDynamic(warmBootState[kSwSwitch]);
  // TODO(ccpowers): remove this loop once we've fully rolled out the
  // new configuration, and we store the profile ID in the WB cache
  for (auto port : *dumpedSwSwitchState_->getPorts()) {
    auto platformPort = hw_->getPlatform()->getPlatformPort(port->getID());
    port->setProfileId(platformPort->getProfileIDBySpeed(port->getSpeed()));
  }
  dumpedSwSwitchState_->publish();
  CHECK(dumpedSwSwitchState_)
      << "Was not able to recover software state after warmboot";

  // Extract ecmps for dumped host table
  auto& hostTable = warmBootState[kHwSwitch][kHostTable];
  for (const auto& ecmpEntry : hostTable[kEcmpHosts]) {
    auto ecmpEgressId = ecmpEntry[kEcmpEgressId].asInt();
    if (ecmpEgressId == BcmEgressBase::INVALID) {
      continue;
    }
    // If the entry is valid, then there must be paths associated with it.
    for (auto path : ecmpEntry[kEcmpEgress][kPaths]) {
      EgressId e = path.asInt();
      hwSwitchEcmp2EgressIds_[ecmpEgressId].insert(e);
    }
  }
  // Extract ecmps from dumped warm boot cache. We
  // may have shut down before a FIB sync
  auto& ecmpObjects = warmBootState[kHwSwitch][kWarmBootCache][kEcmpObjects];
  for (const auto& ecmpEntry : ecmpObjects) {
    auto ecmpEgressId = ecmpEntry[kEcmpEgressId].asInt();
    CHECK(ecmpEgressId != BcmEgressBase::INVALID);
    for (const auto& path : ecmpEntry[kPaths]) {
      EgressId e = path.asInt();
      hwSwitchEcmp2EgressIds_[ecmpEgressId].insert(e);
    }
  }
  XLOG(DBG1) << "Reconstructed following ecmp path map ";
  for (auto& ecmpIdAndEgress : hwSwitchEcmp2EgressIds_) {
    XLOG(DBG1) << ecmpIdAndEgress.first << " (from warmboot file) ==> "
               << toEgressIdsStr(ecmpIdAndEgress.second);
  }

  auto& wbCache = warmBootState[kHwSwitch][kWarmBootCache];
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

  // Extract BcmHost and its egress object from the warm boot file
  for (const auto& hostEntry : hostTable[kHosts]) {
    auto egressId = hostEntry[kEgressId].asInt();
    if (egressId == BcmEgressBase::INVALID) {
      continue;
    }
    egressIdsInWarmBootFile_.insert(egressId);

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

  // extract MPLS next hop and its egress object from the  warm boot file
  const auto& mplsNextHops = (warmBootState[kHwSwitch].find(kMplsNextHops) !=
                              warmBootState[kHwSwitch].items().end())
      ? warmBootState[kHwSwitch][kMplsNextHops]
      : folly::dynamic::array();

  for (const auto& mplsNextHop : mplsNextHops) {
    auto egressId = mplsNextHop[kEgressId].asInt();
    if (egressId == BcmEgressBase::INVALID) {
      continue;
    }
    egressIdsInWarmBootFile_.insert(egressId);
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

  // get l3 intfs for each known vlan in warmboot state file
  // TODO(pshaikh): in earlier warm boot state file, kIntfTable could be
  // absent after two pushes this condition can be removed
  const auto& intfTable = (warmBootState[kHwSwitch].find(kIntfTable) !=
                           warmBootState[kHwSwitch].items().end())
      ? warmBootState[kHwSwitch][kIntfTable]
      : folly::dynamic::array();
  for (const auto& intfTableEntry : intfTable) {
    vlan2BcmIfIdInWarmBootFile_.emplace(
        VlanID(intfTableEntry[kVlan].asInt()), intfTableEntry[kIntfId].asInt());
  }

  // TODO(pshaikh): in earlier warm boot state file, kQosPolicyTable could be
  // absent after two pushes this condition can be removed
  const auto& qosPolicyTable =
      (warmBootState[kHwSwitch].find(kQosPolicyTable) !=
       warmBootState[kHwSwitch].items().end())
      ? warmBootState[kHwSwitch][kQosPolicyTable]
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

void BcmWarmBootCache::populate(std::optional<folly::dynamic> warmBootState) {
  if (warmBootState) {
    populateFromWarmBootState(*warmBootState);
  } else {
    populateFromWarmBootState(getWarmBootState());
  }
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
      rv = bcm_l2_station_get(hw_->getUnit(), l3Intf.l3a_vid, &l2Station);
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
  rv = bcm_l3_egress_ecmp_traverse(
      hw_->getUnit(), ecmpEgressTraversalCallback, this);
  bcmCheckError(rv, "Failed to traverse ecmp egress");

  // populate acls, acl stats
  populateAcls(
      kACLFieldGroupID,
      this->aclEntry2AclStat_,
      this->priority2BcmAclEntryHandle_);

  populateRtag7State();
  populateMirrors();
  populateMirroredPorts();
  populateQosMaps();
  populateLabelSwitchActions();
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
  // Look up egressId in egressIdsInWarmBootFile_
  // to populate both dropEgressId_ and toCPUEgressId_.
  auto egressIdItr = cache->egressIdsInWarmBootFile_.find(egressId);
  if (egressIdItr != cache->egressIdsInWarmBootFile_.cend()) {
    // May be: Add information to figure out how many host or route entry
    // reference it.
    XLOG(DBG1) << "Adding bcm egress entry for: " << *egressIdItr
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

int BcmWarmBootCache::ecmpEgressTraversalCallback(
    int /*unit*/,
    bcm_l3_egress_ecmp_t* ecmp,
    int intfCount,
    bcm_if_t* intfArray,
    void* userData) {
  BcmWarmBootCache* cache = static_cast<BcmWarmBootCache*>(userData);
  EgressIds egressIds;
  // Rather than using the egressId in the intfArray we use the
  // egressIds that we dumped as part of the warm boot state. IntfArray
  // does not include any egressIds that go over the ports that may be
  // down while the warm boot state we dumped does
  try {
    egressIds = cache->getPathsForEcmp(ecmp->ecmp_intf);
  } catch (const FbossError& ex) {
    // There was a bug in SDK where sometimes we got callback with invalid
    // ecmp id with zero number of interfaces. This happened for double wide
    // ECMP entries (when two "words" are used to represent one ECMP entry).
    // For example, if the entries were 200256 and 200258, we got callback
    // for 200257 also with zero interfaces associated with it. If this is
    // the case, we skip this entry.
    //
    // We can also get intfCount of zero with valid ecmp entry (when all the
    // links associated with egress of the ecmp are down. But in this case,
    // cache->getPathsForEcmp() call above should return a valid set of
    // egressIds.
    if (intfCount == 0) {
      return 0;
    }
    throw ex;
  }
  EgressIds egressIdsInHw;
  egressIdsInHw = cache->toEgressIds(intfArray, intfCount);
  XLOG(DBG1) << "ignoring paths for ecmp egress " << ecmp->ecmp_intf
             << " gotten from hardware: " << toEgressIdsStr(egressIdsInHw);

  CHECK(egressIds.size() > 0)
      << "There must be at least one egress pointed to by the ecmp egress id: "
      << ecmp->ecmp_intf;
  CHECK(cache->egressIds2Ecmp_.find(egressIds) == cache->egressIds2Ecmp_.end())
      << "Got a duplicated call for ecmp id: " << ecmp->ecmp_intf
      << " referencing: " << toEgressIdsStr(egressIds);
  cache->egressIds2Ecmp_[egressIds] = *ecmp;
  XLOG(DBG1) << "Added ecmp egress id : " << ecmp->ecmp_intf
             << " pointing to : " << toEgressIdsStr(egressIds) << " egress ids";
  return 0;
}

std::string BcmWarmBootCache::toEgressIdsStr(const EgressIds& egressIds) {
  std::stringstream ss;
  int i = 0;
  for (const auto& egressId : egressIds) {
    ss << egressId;
    ss << (++i < egressIds.size() ? ", " : "");
  }
  return ss.str();
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
  for (auto vrfPfxAndRoute : vrfPrefix2Route_) {
    XLOG(DBG1) << "Deleting unreferenced route in vrf:"
               << std::get<0>(vrfPfxAndRoute.first)
               << " for prefix : " << std::get<1>(vrfPfxAndRoute.first) << "/"
               << std::get<2>(vrfPfxAndRoute.first);
    auto rv = bcm_l3_route_delete(hw_->getUnit(), &(vrfPfxAndRoute.second));
    bcmLogFatal(
        rv,
        hw_,
        "failed to delete unreferenced route in vrf:",
        std::get<0>(vrfPfxAndRoute.first),
        " for prefix : ",
        std::get<1>(vrfPfxAndRoute.first),
        "/",
        std::get<2>(vrfPfxAndRoute.first));
  }
  vrfPrefix2Route_.clear();
  for (auto vrfIPAndRoute : vrfAndIP2Route_) {
    XLOG(DBG1) << "Deleting fully qualified unreferenced route in vrf: "
               << vrfIPAndRoute.first.first
               << " prefix: " << vrfIPAndRoute.first.second;
    auto rv = bcm_l3_route_delete(hw_->getUnit(), &(vrfIPAndRoute.second));
    bcmLogFatal(
        rv,
        hw_,
        "failed to delete fully qualified unreferenced route in vrf: ",
        vrfIPAndRoute.first.first,
        " prefix: ",
        vrfIPAndRoute.first.second);
  }
  vrfAndIP2Route_.clear();

  // purge any lingering label FIB entries
  removeUnclaimedLabelSwitchActions();

  // Delete bcm host entries. Nobody references bcm hosts, but
  // hosts reference egress objects
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

  // Both routes and host entries (which have been deleted earlier) can refer
  // to ecmp egress objects.  Ecmp egress objects in turn refer to egress
  // objects which we delete later
  for (auto idsAndEcmp : egressIds2Ecmp_) {
    auto& ecmp = idsAndEcmp.second;
    XLOG(DBG1) << "Deleting ecmp egress object  " << ecmp.ecmp_intf
               << " pointing to : " << toEgressIdsStr(idsAndEcmp.first);
    auto rv = bcm_l3_egress_ecmp_destroy(hw_->getUnit(), &ecmp);
    bcmLogFatal(
        rv,
        hw_,
        "failed to destroy ecmp egress object :",
        ecmp.ecmp_intf,
        " referring to ",
        toEgressIdsStr(idsAndEcmp.first));
  }
  egressIds2Ecmp_.clear();

  // Delete bcm egress entries. These are referenced by routes, ecmp egress
  // and host objects all of which we deleted above. Egress objects in turn
  // my point to a interface which we delete later
  for (auto egressIdAndEgress : egressId2Egress_) {
    // This is not used yet
    XLOG(DBG1) << "Deleting egress object: " << egressIdAndEgress.first;
    auto rv = bcm_l3_egress_destroy(hw_->getUnit(), egressIdAndEgress.first);
    bcmLogFatal(
        rv, hw_, "failed to destroy egress object ", egressIdAndEgress.first);
  }
  egressId2Egress_.clear();

  // delete any MPLS tunnels
  removeUnclaimedLabeledTunnels();

  // Delete interfaces
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
  // Delete stations
  for (auto vlanAndStation : vlan2Station_) {
    XLOG(DBG1) << "Deleting station for vlan : " << vlanAndStation.first;
    auto rv = bcm_l2_station_delete(hw_->getUnit(), vlanAndStation.first);
    bcmLogFatal(
        rv, hw_, "failed to delete station for vlan : ", vlanAndStation.first);
  }
  vlan2Station_.clear();
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

  egressIdsInWarmBootFile_.clear();
  vrfIp2EgressFromBcmHostInWarmBootFile_.clear();

  // Detach the unclaimed bcm acl stats
  flat_set<BcmAclStatHandle> statsUsed;
  for (auto aclStatItr = aclEntry2AclStat_.begin();
       aclStatItr != aclEntry2AclStat_.end();
       ++aclStatItr) {
    auto& aclStatStatus = aclStatItr->second;
    if (!aclStatStatus.claimed) {
      XLOG(DBG1) << "Detaching unclaimed acl_stat=" << aclStatStatus.stat
                 << "from acl=" << aclStatItr->first;
      detachBcmAclStat(aclStatItr->first, aclStatStatus.stat);
    } else {
      statsUsed.insert(aclStatStatus.stat);
    }
  }

  // Delete the unclaimed bcm acl stats
  for (auto statItr : aclEntry2AclStat_) {
    auto statHandle = statItr.second.stat;
    if (statsUsed.find(statHandle) == statsUsed.end()) {
      XLOG(DBG1) << "Deleting unclaimed acl_stat=" << statHandle;
      removeBcmAclStat(statHandle);
      // add the stat to the set to prevent this loop from attempting to
      // delete the same stat twice
      statsUsed.insert(statHandle);
    }
  }
  aclEntry2AclStat_.clear();

  // Delete acls, since acl(field process) doesn't support
  // bcm, we call BcmAclTable to remove the unclaimed acls
  XLOG(DBG1) << "Unclaimed acl count=" << priority2BcmAclEntryHandle_.size();
  for (auto aclItr : priority2BcmAclEntryHandle_) {
    XLOG(DBG1) << "Deleting unclaimed acl: prio=" << aclItr.first
               << ", handle=" << aclItr.second;
    removeBcmAcl(aclItr.second);
  }
  priority2BcmAclEntryHandle_.clear();

  /* remove unclaimed mirrors and mirrored ports/acls, if any */
  checkUnclaimedMirrors();
  checkUnclaimedQosMaps();
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
      unit, BcmRtag7Module::kTrunkOutputSelectionControl());
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

BcmWarmBootCache::QosMapsItr BcmWarmBootCache::findIngressDscpMap(
    const DscpMap::QosAttributeToTrafficClassSet& dscpToTrafficClassSet) {
  return std::find_if(
      qosMaps_.begin(),
      qosMaps_.end(),
      [dscpToTrafficClassSet](
          const std::unique_ptr<BcmQosMap>& bcmQosMap) -> bool {
        if (bcmQosMap->getType() != BcmQosMap::Type::IP_INGRESS ||
            bcmQosMap->size() != dscpToTrafficClassSet.size()) {
          return false;
        }
        for (const auto& dscpToTrafficClass : dscpToTrafficClassSet) {
          if (!bcmQosMap->ruleExists(
                  dscpToTrafficClass.trafficClass(),
                  dscpToTrafficClass.attr())) {
            return false;
          }
        }
        return true;
      });
}

BcmWarmBootCache::QosMapsItr BcmWarmBootCache::findIngressExpMap(
    const ExpMap::QosAttributeToTrafficClassSet& expToTrafficClassSet) {
  return std::find_if(
      qosMaps_.begin(),
      qosMaps_.end(),
      [expToTrafficClassSet](
          const std::unique_ptr<BcmQosMap>& bcmQosMap) -> bool {
        if (bcmQosMap->getType() != BcmQosMap::Type::MPLS_INGRESS ||
            bcmQosMap->size() != expToTrafficClassSet.size()) {
          return false;
        }
        for (const auto& expToTrafficClass : expToTrafficClassSet) {
          if (!bcmQosMap->ruleExists(
                  expToTrafficClass.trafficClass(), expToTrafficClass.attr())) {
            return false;
          }
        }
        return true;
      });
}

BcmWarmBootCache::QosMapsItr BcmWarmBootCache::findEgressExpMap(
    const ExpMap::QosAttributeToTrafficClassSet& trafficClassToExpSet) {
  return std::find_if(
      qosMaps_.begin(),
      qosMaps_.end(),
      [trafficClassToExpSet](
          const std::unique_ptr<BcmQosMap>& bcmQosMap) -> bool {
        if (bcmQosMap->getType() != BcmQosMap::Type::MPLS_EGRESS
            /*TODO(pshaikh): uncomment below when T59958035 is resolved */
            /*|| bcmQosMap->size() != trafficClassToExpSet.size() */
        ) {
          return false;
        }
        for (const auto& trafficClassToExp : trafficClassToExpSet) {
          if (!bcmQosMap->ruleExists(
                  trafficClassToExp.trafficClass(), trafficClassToExp.attr())) {
            return false;
          }
        }
        return true;
      });
}

void BcmWarmBootCache::checkUnclaimedQosMaps() {
  qosMaps_.clear();
  CHECK_EQ(qosMapKey2QosMapId_.size(), 0) << "unclaimed qos map entries found";
}

void BcmWarmBootCache::programmed(QosMapsItr itr) {
  XLOG(DBG1) << "Programmed QosMap, removing from warm boot cache.";
  qosMaps_.erase(itr);
}

BcmWarmBootCache::QosMapKey2QosMapIdItr BcmWarmBootCache::findQosMap(
    const std::string& policyName,
    BcmQosMap::Type type) {
  return qosMapKey2QosMapId_.find(std::make_pair(policyName, type));
}

void BcmWarmBootCache::programmed(BcmWarmBootCache::QosMapKey2QosMapIdItr itr) {
  XLOG(DBG1) << "Programmed QosMap of type " << itr->first.second << " with id "
             << itr->second << " for policy " << itr->first.first
             << ", removing from warm boot cache.";
  qosMapKey2QosMapId_.erase(itr);
}

} // namespace facebook::fboss
