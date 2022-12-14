/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

extern "C" {
#include <bcm/l2.h>
#include <bcm/l3.h>
#include <bcm/port.h>
#include <bcm/vlan.h>
}

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <folly/Conv.h>
#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/container/F14Map.h>
#include <folly/dynamic.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <algorithm>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "fboss/agent/hw/bcm/BcmMirror.h"
#include "fboss/agent/hw/bcm/BcmQosMap.h"
#include "fboss/agent/hw/bcm/BcmRouteCounter.h"
#include "fboss/agent/hw/bcm/BcmRtag7Module.h"
#include "fboss/agent/hw/bcm/BcmTeFlowTable.h"
#include "fboss/agent/hw/bcm/BcmTrunk.h"
#include "fboss/agent/hw/bcm/BcmTypes.h"
#include "fboss/agent/hw/bcm/BcmWarmBootState.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/QosPolicy.h"
#include "fboss/agent/state/RouteTypes.h"

namespace facebook::fboss {

class AclMap;
class BcmSwitchIf;
class InterfaceMap;
class LoadBalancerMap;
class QosPolicyMap;
class SwitchState;
class Vlan;
struct MirrorTunnel;
class VlanMap;
class MirrorMap;
class PortMap;

class BcmWarmBootCache {
 public:
  explicit BcmWarmBootCache(const BcmSwitchIf* hw);
  folly::dynamic getWarmBootStateFollyDynamic() const;
  void populate(
      const folly::dynamic& warmBootState,
      std::optional<state::WarmbootState> thriftState);
  struct VlanInfo {
    VlanInfo(
        VlanID _vlan,
        bcm_pbmp_t _untagged,
        bcm_pbmp_t _allPorts,
        InterfaceID _intfID)
        : vlan(_vlan), intfID(_intfID) {
      BCM_PBMP_ASSIGN(untagged, _untagged);
      BCM_PBMP_ASSIGN(allPorts, _allPorts);
    }

    // This constructor defaults to setting the interfaceID to be the same
    // as the vlan ID. This is our current behavior. If we want to support
    // interface IDs that are different than their corresponding vlan ID
    // the various call-sites need to use the other constructor.
    VlanInfo(VlanID _vlan, bcm_pbmp_t _untagged, bcm_pbmp_t _allPorts)
        : VlanInfo(_vlan, _untagged, _allPorts, InterfaceID(_vlan)) {}

    VlanID vlan;
    bcm_pbmp_t untagged;
    bcm_pbmp_t allPorts;
    InterfaceID intfID;
  };
  typedef bcm_if_t EcmpEgressId;
  typedef bcm_if_t EgressId;
  typedef boost::container::flat_map<EgressId, uint64_t> EgressId2Weight;
  typedef boost::container::flat_map<EcmpEgressId, EgressId2Weight>
      Ecmp2EgressIds;
  template <typename T>
  static EgressId2Weight toEgressId2Weight(T* egress, int count);
  static std::string toEgressId2WeightStr(
      const EgressId2Weight& egressId2Weight);
  /*
   * Get all cached ecmp egress Ids
   */
  const Ecmp2EgressIds& ecmp2EgressIds() const {
    return hwSwitchEcmp2EgressIds_;
  }

 private:
  using Egress = bcm_l3_egress_t;
  using EcmpEgress = bcm_l3_egress_ecmp_t;
  using IntfId = bcm_if_t;
  using EgressIdAndEgress = std::pair<EgressId, Egress>;
  using VlanAndMac = std::pair<VlanID, folly::MacAddress>;
  using IntfIdAndMac = std::pair<IntfId, folly::MacAddress>;
  using HostKey =
      std::tuple<bcm_vrf_t, folly::IPAddress, std::optional<bcm_if_t>>;
  /*
   * VRF, IP, Mask
   * TODO - Convert mask to mask len for efficient storage/lookup
   */
  typedef std::tuple<bcm_vrf_t, folly::IPAddress, folly::IPAddress>
      VrfAndPrefix;
  typedef std::pair<bcm_vrf_t, folly::IPAddress> VrfAndIP;
  /*
   * Cache containers
   */
  typedef boost::container::flat_map<VlanID, VlanInfo> Vlan2VlanInfo;
  typedef boost::container::flat_map<VlanID, bcm_l2_station_t> Vlan2Station;
  typedef boost::container::flat_map<VlanAndMac, bcm_l3_intf_t> VlanAndMac2Intf;
  typedef boost::container::flat_map<VlanID, bcm_if_t>
      Vlan2BcmIfIdInWarmBootFile;

  typedef folly::F14FastMap<VrfAndIP, bcm_l3_host_t> VrfAndIP2Host;
  typedef folly::F14FastMap<VrfAndPrefix, bcm_l3_route_t> VrfAndPrefix2Route;
  typedef boost::container::flat_map<EgressId2Weight, EcmpEgress>
      EgressIds2Ecmp;
  using VrfAndIP2Route = boost::container::flat_map<VrfAndIP, bcm_l3_route_t>;
  using EgressId2Egress = boost::container::flat_map<EgressId, Egress>;
  using HostTableInWarmBootFile = boost::container::flat_map<HostKey, EgressId>;
  using MplsNextHop2EgressIdInWarmBootFile =
      boost::container::flat_map<BcmLabeledHostKey, EgressId>;
  using LabelStackKey =
      std::pair<bcm_vlan_t, LabelForwardingAction::LabelStack>;
  using LabelStackKey2TunnelId =
      boost::container::flat_map<LabelStackKey, bcm_if_t>;

  // current h/w acls: key = priority, value = BcmAclEntryHandle
  using Priority2BcmAclEntryHandle =
      boost::container::flat_map<int, BcmAclEntryHandle>;
  using Index2ReasonToQueue =
      boost::container::flat_map<int, cfg::PacketRxReasonToQueue>;
  using MirrorEgressPath2Handle = boost::container::flat_map<
      std::pair<bcm_gport_t, std::optional<MirrorTunnel>>,
      BcmMirrorHandle>;
  using MirroredPort2Handle = boost::container::
      flat_map<std::pair<bcm_gport_t, uint32_t>, BcmMirrorHandle>;
  using MirroredAcl2Handle = boost::container::
      flat_map<std::pair<BcmAclEntryHandle, MirrorDirection>, BcmMirrorHandle>;
  using Trunks = boost::container::flat_map<AggregatePortID, bcm_trunk_t>;
  using QosMapIds = boost::container::flat_set<int>;

  // MPLS action
  using Label2LabelActionMap = boost::container::
      flat_map<bcm_mpls_label_t, std::unique_ptr<BcmMplsTunnelSwitchT>>;
  struct AclStatStatus {
    BcmAclStatHandle stat{-1};
    bool claimed{false};
  };
  // current h/w acl stats: key = BcmAclEntryHandle, value = AclStatStatus
  using AclEntry2AclStat =
      boost::container::flat_map<BcmAclEntryHandle, AclStatStatus>;

  struct TeFlowStatStatus {
    BcmTeFlowStatHandle stat{-1};
    BcmAclStatActionIndex actionIndex{BcmAclStat::kDefaultAclActionIndex};
    bool claimed{false};
  };
  // current h/w teflow stats: key = BcmTeFlowEntryHandle, value =
  // TeFlowStatStatus
  using TeFlowEntry2TeFlowStat =
      std::map<BcmTeFlowEntryHandle, TeFlowStatStatus>;

  using QosMapKey = std::pair<std::string, BcmQosMap::Type>;
  using QosMapKey2QosMapId =
      boost::container::flat_map<QosMapKey, BcmQosPolicyHandle>;
  using QosMapId2QosMap =
      std::map<BcmQosPolicyHandle, std::unique_ptr<BcmQosMap>>;
  using QosMapId2QosMapItr = typename QosMapId2QosMap::iterator;

  // Route counter id to hw counter id map
  typedef boost::container::flat_map<RouteCounterID, BcmRouteCounterID>
      RouteCounterIDMap;
  using RouteCounterIDMapItr = typename RouteCounterIDMap::const_iterator;

  // current h/w TeFlows: key = TeFlowMapKey, value = BcmTeFlowEntryHandle
  using TeFlowMapKey = std::pair<bcm_port_t, folly::IPAddress>;
  using TeFlow2BcmTeFlowEntryHandle =
      std::map<TeFlowMapKey, BcmTeFlowEntryHandle>;

  using UdfGroupInfoPair = std::pair<bcm_udf_id_t, bcm_udf_t>;
  using UdfGroupNameToInfoMap = std::map<std::string, UdfGroupInfoPair>;
  using UdfGroupNameToInfoMapItr =
      typename UdfGroupNameToInfoMap::const_iterator;
  using UdfGroupPacketMatcherMap =
      std::map<std::string, bcm_udf_pkt_format_id_t>;
  using UdfGroupNameToPacketMatcherMap =
      std::map<std::string, UdfGroupPacketMatcherMap>;
  using UdfGroupNameToPacketMatcherMapItr =
      typename UdfGroupNameToPacketMatcherMap::const_iterator;
  using UdfPktMatcherInfoPair =
      std::pair<bcm_udf_pkt_format_id_t, bcm_udf_pkt_format_info_t>;
  using UdfPktMatcherNameToInfoMap =
      std::map<std::string, UdfPktMatcherInfoPair>;
  using UdfPktMatcherNameToInfoMapItr =
      typename UdfPktMatcherNameToInfoMap::const_iterator;

  /*
   * Callbacks for traversing entries in BCM h/w tables
   */
  static int hostTraversalCallback(
      int unit,
      int index,
      bcm_l3_host_t* info,
      void* user_data);
  static int egressTraversalCallback(
      int unit,
      EgressId intf,
      bcm_l3_egress_t* info,
      void* user_data);
  static int routeTraversalCallback(
      int unit,
      int index,
      bcm_l3_route_t* info,
      void* userData);
  template <typename T>
  static int ecmpEgressTraversalCallback(
      int unit,
      bcm_l3_egress_ecmp_t* ecmp,
      int memberCount,
      T* memberArray,
      void* userData);

  /**
   * Helper functions for populate AclEntry since we don't have
   * Bcm support for Field Processor
   */
  // retrieve all bcm acls of the specified group
  void populateAcls(
      const int groupId,
      AclEntry2AclStat& stats,
      Priority2BcmAclEntryHandle& acls);
  void
  populateAclStats(int groupID, BcmAclEntryHandle acl, AclEntry2AclStat& stats);
  // remove bcm acl directly from h/w
  void removeBcmAcl(BcmAclEntryHandle handle);

  void populateRtag7State();
  void populateQosMaps();

  void populateMirrors();
  void populateMirroredPorts();
  void populateMirroredPort(bcm_gport_t port);
  void populateMirroredAcl(BcmAclEntryHandle handle);

  void checkUnclaimedMirrors();
  void removeUnclaimedMirror(BcmMirrorHandle mirror);

  void populateLabelSwitchActions();
  void removeUnclaimedLabelSwitchActions();

  void populateLabelStack2TunnelId(bcm_l3_egress_t* egress);
  void removeUnclaimedLabeledTunnels();

  void removeUnclaimedRoutes();
  void removeUnclaimedHostEntries();
  void removeUnclaimedEcmpEgressObjects();
  void removeUnclaimedEgressObjects();

  void removeUnclaimedInterfaces();
  void removeUnclaimedStations();
  void removeUnclaimedVlans();

  void removeUnclaimedAclStats();
  void removeUnclaimedAcls();

  void removeUnclaimedTeFlowStats();
  void removeUnclaimedTeFlows();

  void removeUnclaimedCosqMappings();
  void checkUnclaimedQosMaps();

  void populateSwitchSettings();

  void populateRxReasonToQueue();

  // retrieve all bcm teflows of the specified group
  void populateTeFlows(
      const int groupId,
      TeFlowEntry2TeFlowStat& stats,
      TeFlow2BcmTeFlowEntryHandle& teflows);
  void populateTeFlowStats(
      int groupID,
      BcmTeFlowEntryHandle teflow,
      TeFlowEntry2TeFlowStat& stats);

  // remove bcm teflow directly from h/w
  void removeBcmTeFlow(BcmTeFlowEntryHandle handle);

 public:
  /*
   * Iterators and find functions for finding VlanInfo
   */
  typedef Vlan2VlanInfo::const_iterator Vlan2VlanInfoCitr;
  Vlan2VlanInfoCitr vlan2VlanInfo_beg() const {
    return vlan2VlanInfo_.begin();
  }
  Vlan2VlanInfoCitr vlan2VlanInfo_end() const {
    return vlan2VlanInfo_.end();
  }
  Vlan2VlanInfoCitr findVlanInfo(VlanID vlan) const {
    return vlan2VlanInfo_.find(vlan);
  }
  void programmed(Vlan2VlanInfoCitr vitr) {
    XLOG(DBG1) << "Programmed vlan: " << vitr->first
               << " removing from warm boot cache";
    vlan2VlanInfo_.erase(vitr);
  }
  /*
   * Iterators and find functions for finding bcm_l2_station_t
   */
  typedef Vlan2Station::const_iterator Vlan2StationCitr;
  Vlan2StationCitr vlan2Station_beg() const {
    return vlan2Station_.begin();
  }
  Vlan2StationCitr vlan2Station_end() const {
    return vlan2Station_.end();
  }
  Vlan2StationCitr findVlanStation(VlanID vlan) {
    return vlan2Station_.find(vlan);
  }
  void programmed(Vlan2StationCitr vsitr) {
    XLOG(DBG1) << "Programmed station : " << vsitr->first
               << " removing from warm boot cache";
    vlan2Station_.erase(vsitr);
  }
  /*
   * Iterators and find functions for finding bcm_l3_intf_t
   */
  typedef VlanAndMac2Intf::const_iterator VlanAndMac2IntfCitr;
  VlanAndMac2IntfCitr vlanAndMac2Intf_beg() const {
    return vlanAndMac2Intf_.begin();
  }
  VlanAndMac2IntfCitr vlanAndMac2Intf_end() const {
    return vlanAndMac2Intf_.end();
  }
  VlanAndMac2IntfCitr findL3Intf(VlanID vlan, folly::MacAddress mac) {
    return vlanAndMac2Intf_.find(VlanAndMac(vlan, mac));
  }
  void programmed(VlanAndMac2IntfCitr vmitr) {
    XLOG(DBG1) << "Programmed interface in vlan : " << vmitr->first.first
               << " and mac: " << vmitr->first.second
               << " removing from warm boot cache";
    vlanAndMac2Intf_.erase(vmitr);
  }
  /*
   * Iterators and find functions for finding bcm_l3_egress_t
   */
  EgressId getDropEgressId() const {
    return dropEgressId_;
  }
  EgressId getToCPUEgressId() const {
    return toCPUEgressId_;
  }
  /**
   * Iterators and find functions for finding egress objects.
   */
  using EgressId2EgressCitr = EgressId2Egress::const_iterator;
  EgressId2EgressCitr egressId2Egress_begin() {
    return egressId2Egress_.begin();
  }
  EgressId2EgressCitr egressId2Egress_end() {
    return egressId2Egress_.end();
  }
  EgressId2EgressCitr findEgress(EgressId id) const {
    return egressId2Egress_.find(id);
  }
  EgressId2EgressCitr findEgressFromHost(
      bcm_vrf_t vrf,
      const folly::IPAddress& addr,
      std::optional<bcm_if_t> intf);

  EgressId2EgressCitr findEgressFromLabeledHostKey(
      const BcmLabeledHostKey& key);

  void programmed(EgressId2EgressCitr citr) {
    XLOG(DBG1) << "Programmed egress entry: " << citr->first
               << ". Removing from warmboot cache.";
    egressId2Egress_.erase(citr->first);
  }

  using LabelStackKey2TunnelIdCitr = LabelStackKey2TunnelId::const_iterator;
  LabelStackKey2TunnelIdCitr labelStack2TunnelId_begin() const {
    return labelStackKey2TunnelId_.begin();
  }
  LabelStackKey2TunnelIdCitr labelStack2TunnelId_end() const {
    return labelStackKey2TunnelId_.end();
  }
  LabelStackKey2TunnelIdCitr findTunnel(
      bcm_vlan_t vlan,
      const LabelForwardingAction::LabelStack& stack) const {
    return labelStackKey2TunnelId_.find(LabelStackKey(vlan, stack));
  }
  void programmed(LabelStackKey2TunnelIdCitr citr) {
    labelStackKey2TunnelId_.erase(citr->first);
  }

  /*
   * Iterators and find functions for finding bcm_l3_host_t
   */
  typedef VrfAndIP2Host::const_iterator VrfAndIP2HostCitr;
  VrfAndIP2HostCitr vrfAndIP2Host_beg() const {
    return vrfIp2Host_.begin();
  }
  VrfAndIP2HostCitr vrfAndIP2Host_end() const {
    return vrfIp2Host_.end();
  }
  VrfAndIP2HostCitr findHost(bcm_vrf_t vrf, const folly::IPAddress& ip) const {
    return vrfIp2Host_.find(VrfAndIP(vrf, ip));
  }
  void programmed(VrfAndIP2HostCitr vrhitr) {
    XLOG(DBG1) << "Programmed host for vrf : " << vrhitr->first.first
               << " ip : " << vrhitr->first.second
               << " removing from warm boot cache ";
    vrfIp2Host_.erase(vrhitr);
  }
  /*
   * Iterators and find functions for finding bcm_l3_route_t
   */
  typedef VrfAndPrefix2Route::const_iterator VrfAndPfx2RouteCitr;
  VrfAndPfx2RouteCitr vrfAndPrefix2Route_beg() const {
    return vrfPrefix2Route_.begin();
  }
  VrfAndPfx2RouteCitr vrfAndPrefix2Route_end() const {
    return vrfPrefix2Route_.end();
  }
  VrfAndPfx2RouteCitr
  findRoute(bcm_vrf_t vrf, const folly::IPAddress& ip, uint8_t mask) {
    using folly::IPAddress;
    using folly::IPAddressV4;
    using folly::IPAddressV6;
    if (ip.isV6()) {
      return vrfPrefix2Route_.find(VrfAndPrefix(
          vrf, ip, IPAddress(IPAddressV6(IPAddressV6::fetchMask(mask)))));
    }
    return vrfPrefix2Route_.find(VrfAndPrefix(
        vrf, ip, IPAddress(IPAddressV4(IPAddressV4::fetchMask(mask)))));
  }
  void programmed(VrfAndPfx2RouteCitr vrpitr) {
    XLOG(DBG1) << "Programmed route in vrf : " << std::get<0>(vrpitr->first)
               << "  prefix: " << std::get<1>(vrpitr->first) << "/"
               << std::get<2>(vrpitr->first)
               << " removing from warm boot cache ";
    vrfPrefix2Route_.erase(vrpitr);
  }

  /**
   * Iterators and find functions for fully qualified routes in the route table.
   */
  using VrfAndIP2RouteCitr = VrfAndIP2Route::const_iterator;
  VrfAndIP2RouteCitr vrfAndIP2Route_begin() const {
    return vrfAndIP2Route_.begin();
  }
  VrfAndIP2RouteCitr vrfAndIP2Route_end() const {
    return vrfAndIP2Route_.end();
  }
  VrfAndIP2RouteCitr findHostRouteFromRouteTable(
      bcm_vrf_t vrf,
      const folly::IPAddress& ip) const {
    return vrfAndIP2Route_.find(VrfAndIP(vrf, ip));
  }
  void programmed(VrfAndIP2RouteCitr citr) {
    XLOG(DBG1) << "Programmed host route, removing from warm boot cache. "
               << "vrf: " << citr->first.first << " "
               << "ip: " << citr->first.second;
    vrfAndIP2Route_.erase(citr);
  }

  /*
   * Iterators and find functions for bcm_l3_egress_ecmp_t
   */
  typedef EgressIds2Ecmp::const_iterator EgressIds2EcmpCItr;
  EgressIds2EcmpCItr egressIds2Ecmp_begin() {
    return egressIds2Ecmp_.begin();
  }
  EgressIds2EcmpCItr egressIds2Ecmp_end() {
    return egressIds2Ecmp_.end();
  }
  EgressIds2EcmpCItr findEcmp(const EgressId2Weight& egressId2Weight) {
    return egressIds2Ecmp_.find(egressId2Weight);
  }
  void programmed(EgressIds2EcmpCItr eeitr) {
    XLOG(DBG1) << "Programmed ecmp egress: " << eeitr->second.ecmp_intf
               << " removing from warm boot cache";
    // Remove from ecmp->egressId mapping since now a BcmEcmpEgress object
    // exists which has the egress id info.
    //
    // Note: This should be done before erasing the iterator.
    hwSwitchEcmp2EgressIds_.erase(eeitr->second.ecmp_intf);
    egressIds2Ecmp_.erase(eeitr);
  }

  /*
   * Iterators and find functions for acls and acl stats
   */
  using Prio2BcmAclItr = Priority2BcmAclEntryHandle::const_iterator;
  Prio2BcmAclItr priority2BcmAclEntryHandle_begin() {
    return priority2BcmAclEntryHandle_.begin();
  }
  Prio2BcmAclItr priority2BcmAclEntryHandle_end() {
    return priority2BcmAclEntryHandle_.end();
  }
  Prio2BcmAclItr findAcl(int priority) {
    return priority2BcmAclEntryHandle_.find(priority);
  }
  void programmed(Prio2BcmAclItr itr) {
    XLOG(DBG1) << "Programmed AclEntry, removing from warm boot cache. "
               << "priority=" << itr->first << " acl entry=" << itr->second;
    priority2BcmAclEntryHandle_.erase(itr);
  }

  /*
   * Iterators and find functions for teflows
   */
  using TeFlow2BcmTeFlowItr = TeFlow2BcmTeFlowEntryHandle::const_iterator;
  TeFlow2BcmTeFlowItr teFlow2BcmTeFlowEntryHandle_begin() {
    return teFlow2BcmTeFlowEntryHandle_.begin();
  }
  TeFlow2BcmTeFlowItr teFlow2BcmTeFlowEntryHandle_end() {
    return teFlow2BcmTeFlowEntryHandle_.end();
  }
  TeFlow2BcmTeFlowItr findTeFlow(uint16_t srcPort, folly::IPAddress destIp) {
    return teFlow2BcmTeFlowEntryHandle_.find({srcPort, destIp});
  }
  void programmed(TeFlow2BcmTeFlowItr itr) {
    XLOG(DBG1) << "Programmed TeFlowEntry, removing from warm boot cache. "
               << "Teflow: srcPort=" << itr->first.first
               << " destIp= " << itr->first.second
               << " teflow entry=" << itr->second;
    teFlow2BcmTeFlowEntryHandle_.erase(itr);
  }

  typedef Index2ReasonToQueue::const_iterator Index2ReasonToQueueCItr;
  Index2ReasonToQueueCItr index2ReasonToQueue_begin() const {
    return index2ReasonToQueue_.begin();
  }
  Index2ReasonToQueueCItr index2ReasonToQueue_end() const {
    return index2ReasonToQueue_.end();
  }
  Index2ReasonToQueueCItr findReasonToQueue(const int index) const {
    return index2ReasonToQueue_.find(index);
  }
  void programmed(Index2ReasonToQueueCItr itr) {
    XLOG(DBG1) << "Programmed reason to queue at index: " << itr->first << " "
               << "rxReason: "
               << apache::thrift::util::enumName(*itr->second.rxReason())
               << " queueId: " << *itr->second.queueId();
    index2ReasonToQueue_.erase(itr);
  }

  bool unitControlMatches(
      char module,
      bcm_switch_control_t switchControl,
      int arg) const;
  void programmed(char module, bcm_switch_control_t switchControl);
  bool unitControlMatches(
      LoadBalancerID loadBalancerID,
      int switchControl,
      int arg) const;
  void programmed(LoadBalancerID loadBalancerID, int switchControl);

  using AclEntry2AclStatItr = AclEntry2AclStat::iterator;
  AclEntry2AclStatItr AclEntry2AclStat_end() {
    return aclEntry2AclStat_.end();
  }
  AclEntry2AclStatItr findAclStat(const BcmAclEntryHandle& bcmAclEntry);
  void programmed(AclEntry2AclStatItr itr);

  using TeFlowEntry2TeFlowStatItr = TeFlowEntry2TeFlowStat::iterator;
  TeFlowEntry2TeFlowStatItr TeFlowEntry2TeFlowStat_end() {
    return teFlowEntry2TeFlowStat_.end();
  }
  TeFlowEntry2TeFlowStatItr findTeFlowStat(
      const BcmTeFlowEntryHandle& bcmTeFlowEntry);
  void programmed(TeFlowEntry2TeFlowStatItr itr);

  /*
   * Iterators and find functions for trunks
   */
  using TrunksItr = Trunks::iterator;
  TrunksItr trunks_begin() {
    return trunks_.begin();
  }
  TrunksItr trunks_end() {
    return trunks_.end();
  }
  void programmed(TrunksItr itr);

  /*
   * owner is done programming its entries remove any entries
   * from hw that had owner as their only remaining owner
   */
  void clear();
  bool fillVlanPortInfo(Vlan* vlan);
  /*
   * Serialize to folly::dynamic
   */
  folly::dynamic toFollyDynamic() const;

  const BcmSwitchIf* getHw() const {
    return hw_;
  }

  using MirrorEgressPath2HandleCitr =
      typename MirrorEgressPath2Handle::const_iterator;
  MirrorEgressPath2HandleCitr mirrorsBegin() const;
  MirrorEgressPath2HandleCitr mirrorsEnd() const;
  MirrorEgressPath2HandleCitr findMirror(
      bcm_gport_t port,
      const std::optional<MirrorTunnel>& tunnel) const;
  void programmedMirror(MirrorEgressPath2HandleCitr itr);

  using MirroredPort2HandleCitr = typename MirroredPort2Handle::const_iterator;
  MirroredPort2HandleCitr mirroredPortsBegin() const;
  MirroredPort2HandleCitr mirroredPortsEnd() const;
  MirroredPort2HandleCitr findMirroredPort(bcm_gport_t port, uint32_t flags)
      const;
  void programmedMirroredPort(MirroredPort2HandleCitr itr);
  bool isSflowMirror(BcmMirrorHandle handle) const;

  using MirroredAcl2HandleCitr = typename MirroredAcl2Handle::const_iterator;
  MirroredAcl2HandleCitr mirroredAclsBegin() const;
  MirroredAcl2HandleCitr mirroredAclsEnd() const;
  MirroredAcl2HandleCitr findMirroredAcl(
      BcmAclEntryHandle entry,
      MirrorDirection direction) const;
  void programmedMirroredAcl(MirroredAcl2HandleCitr itr);

  const SwitchState& getDumpedSwSwitchState() const {
    return *dumpedSwSwitchState_;
  }

  using Label2LabelActionMapCitr =
      typename Label2LabelActionMap::const_iterator;
  Label2LabelActionMapCitr Label2LabelActionMapBegin() const {
    return label2LabelActions_.begin();
  }
  Label2LabelActionMapCitr Label2LabelActionMapEnd() const {
    return label2LabelActions_.end();
  }

  Label2LabelActionMapCitr findLabelAction(bcm_mpls_label_t label) const {
    return label2LabelActions_.find(label);
  }
  void programmedLabelAction(Label2LabelActionMapCitr itr) {
    label2LabelActions_.erase(itr);
  }

  BcmWarmBootState* getBcmWarmBootState() const {
    /*
     * In-memory, and current BCM switch warm boot state.
     * this is a handle to an object which can generate folly::dynamic for only
     * pieces of subset of current BcmSwitch data structures. These are
     * typically those pieces which are unavailable through SDK traversal APIs
     * or which are needed to be maintained for absence of some entries in ASIC
     * e.g. for correct egress association as link local host entries are not
     * saved in ASIC but maintained in software.
     */
    return bcmWarmBootState_.get();
  }

  QosMapId2QosMapItr findQosMap(
      const std::shared_ptr<QosPolicy>& qosPolicy,
      BcmQosMap::Type type);
  void programmed(
      const std::string& policyName,
      BcmQosMap::Type type,
      QosMapId2QosMapItr itr);
  QosMapId2QosMapItr QosMapId2QosMapBegin() {
    return qosMapId2QosMap_.begin();
  }

  QosMapId2QosMapItr QosMapId2QosMapEnd() {
    return qosMapId2QosMap_.end();
  }

  cfg::L2LearningMode getL2LearningMode() const {
    return l2LearningMode_;
  }

  bool ptpTcSettingMatches(bool enable) const {
    return ptpTcEnabled_.has_value() && *ptpTcEnabled_ == enable;
  }

  bool isUdfInitialized() const {
    return udfEnabled_;
  }

  void ptpTcProgrammed() {
    ptpTcEnabled_ = std::nullopt;
  }

  int getRouteCounterModeId() const {
    return routeCounterModeId_;
  }

  int getRouteCounterV6FlexActionId() const {
    return v6FlexCounterAction_;
  }

  RouteCounterIDMapItr findRouteCounterID(RouteCounterID counterID) const {
    return routeCounterIDs_.find(counterID);
  }

  RouteCounterIDMapItr RouteCounterIDMapEnd() {
    return routeCounterIDs_.end();
  }

  UdfGroupNameToInfoMapItr findUdfGroupInfo(const std::string& name) const {
    return udfGroupNameToInfoMap_.find(name);
  }

  UdfGroupNameToPacketMatcherMapItr findUdfGroupPacketMatcher(
      const std::string& name) const {
    return udfGroupNameToPacketMatcherMap_.find(name);
  }

  UdfPktMatcherNameToInfoMapItr findUdfPktMatcherInfo(
      const std::string& name) const {
    return udfPktMatcherNameToInfoMap_.find(name);
  }

  UdfGroupNameToInfoMapItr UdfGroupNameToInfoMapEnd() {
    return udfGroupNameToInfoMap_.end();
  }

  UdfGroupNameToPacketMatcherMapItr UdfGroupNameToPacketMatcherMapEnd() {
    return udfGroupNameToPacketMatcherMap_.end();
  }

  UdfPktMatcherNameToInfoMapItr UdfPktMatcherNameToInfoMapEnd() {
    return udfPktMatcherNameToInfoMap_.end();
  }

  void programmed(UdfGroupNameToInfoMapItr itr) {
    udfGroupNameToInfoMap_.erase(itr);
  }

  void programmed(UdfGroupNameToPacketMatcherMapItr itr) {
    udfGroupNameToPacketMatcherMap_.erase(itr);
  }

  void programmed(UdfPktMatcherNameToInfoMapItr itr) {
    udfPktMatcherNameToInfoMap_.erase(itr);
  }

  void programmed(RouteCounterIDMapItr itr) {
    routeCounterIDs_.erase(itr);
  }

  int getTeFlowDstIpPrefixLength() const {
    return teFlowDstIpPrefixLength_;
  }

  int getTeFlowHintId() const {
    return teFlowHintId_;
  }

  int getTeFlowGroupId() const {
    return teFlowGroupId_;
  }

  int getTeFlowFlexCounterId() const {
    return teFlowFlexCounterId_;
  }

  void teFlowTableProgrammed() {
    teFlowDstIpPrefixLength_ = 0;
    teFlowHintId_ = 0;
    teFlowGroupId_ = 0;
    teFlowFlexCounterId_ = 0;
  }

 private:
  /*
   * Get egress ids for a ECMP Id. Will throw FbossError
   * if ecmp id is not found in hwSwitchEcmp2EgressIds_
   * map
   */
  const EgressId2Weight& getPathsForEcmp(EgressId ecmp) const;
  void populateFromWarmBootState(
      const folly::dynamic& warmBootState,
      std::optional<state::WarmbootState> thriftState);

  void populateEcmpEntryFromWarmBootState(
      const folly::dynamic& hwWarmBootState);
  void populateTrunksFromWarmBootState(const folly::dynamic& hwWarmBootState);
  void populateRouteCountersFromWarmBootState(
      const folly::dynamic& hwWarmBootState);
  void populateHostEntryFromWarmBootState(const folly::dynamic& hostTable);

  void populateMplsNextHopFromWarmBootState(
      const folly::dynamic& hwWarmBootState);
  void populateIntfFromWarmBootState(const folly::dynamic& hwWarmBootState);
  void populateQosPolicyFromWarmBootState(
      const folly::dynamic& hwWarmBootState);
  void populateTeFlowFromWarmBootState(const folly::dynamic& hwWarmBootState);
  void populateUdfFromWarmBootState(const folly::dynamic& hwWarmBootState);
  void populateUdfGroupFromWarmBootState(const folly::dynamic& udfGroup);
  void populateUdfPacketMatcherFromWarmBootState(
      const folly::dynamic& udfPacketMatcher);

  // No copy or assignment.
  BcmWarmBootCache(const BcmWarmBootCache&) = delete;
  BcmWarmBootCache& operator=(const BcmWarmBootCache&) = delete;
  const BcmSwitchIf* hw_;
  Vlan2VlanInfo vlan2VlanInfo_;
  Vlan2Station vlan2Station_;
  VlanAndMac2Intf vlanAndMac2Intf_;
  Vlan2BcmIfIdInWarmBootFile vlan2BcmIfIdInWarmBootFile_;

  // This is the set of egress ids pointed by BcmHost in warm boot file.
  EgressId2Weight egressId2WeightInWarmBootFile_;
  // Mapping from <vrf, ip, intf> to the egress,
  // based on the BcmHost in warm boot file.
  HostTableInWarmBootFile vrfIp2EgressFromBcmHostInWarmBootFile_;

  // Mapping from Labeled Host Key to Egress ID
  MplsNextHop2EgressIdInWarmBootFile mplsNextHops2EgressIdInWarmBootFile_;

  // The host table in HW
  VrfAndIP2Host vrfIp2Host_;
  // These are routes from defip table that are not fully qualified (not /32 or
  // /128).
  VrfAndPrefix2Route vrfPrefix2Route_;
  // These are the fully qualified routes stored in defip table (/32 and /128
  // routes).
  VrfAndIP2Route vrfAndIP2Route_;
  EgressId2Egress egressId2Egress_;
  EgressIds2Ecmp egressIds2Ecmp_;
  LabelStackKey2TunnelId labelStackKey2TunnelId_;
  bcm_if_t dropEgressId_;
  bcm_if_t toCPUEgressId_;
  // hwSwitchEcmp2EgressIds_ represents what the mapping
  // from ecmp -> set<egressId> in the in memory BcmEcmpEgress
  // entries (and related host tables).
  // This may be distinct from the state in actual h/w since on
  // port down events we immediately remove the corresponding
  // egress ids from the ECMP entry in HW, but not from the BcmEcmpEgress
  // entry in memory. This minimizes the packets lost by quickly
  // shrinking the ECMP group and letting the traffic flow over
  // other up ports.
  // 2 alternate approaches were considered and rejected
  // a) Wait for routing protocols (BGP for e.g.) to notice the port
  // down and time out its peerings. The routing protocol would then
  // update all the routes that have a next hop that is reached over this
  // down port. Fboss agent would then be told about the updated routes,
  // at which point Fboss agent would update the relevant routes and ECMP
  // entries. This is *too slow*
  // b) On noticing the port down update all the routes and ECMP entries
  // etc in FBOSS agent itself. There problems with this is that we may
  // have a lot of routes. So each port down would create a big update.
  // Now if we get back to back port down events, the updates for the
  // second port would be queued behind the updates for the downed port.
  // The delay can be multiple seconds.
  Ecmp2EgressIds hwSwitchEcmp2EgressIds_;

  // acls
  Priority2BcmAclEntryHandle priority2BcmAclEntryHandle_;

  // TeFlows
  TeFlow2BcmTeFlowEntryHandle teFlow2BcmTeFlowEntryHandle_;

  Index2ReasonToQueue index2ReasonToQueue_;

  // trunks
  Trunks trunks_;

  BcmRtag7Module::ModuleState moduleAState_;
  BcmRtag7Module::ModuleState moduleBState_;
  BcmRtag7Module::OutputSelectionState ecmpOutputSelectionState_;
  BcmRtag7Module::OutputSelectionState trunkOutputSelectionState_;

  // acl stats
  AclEntry2AclStat aclEntry2AclStat_;

  // TeFlow  stats
  TeFlowEntry2TeFlowStat teFlowEntry2TeFlowStat_;

  std::unique_ptr<SwitchState> dumpedSwSwitchState_;
  MirrorEgressPath2Handle mirrorEgressPath2Handle_;
  MirroredPort2Handle mirroredPort2Handle_;
  MirroredAcl2Handle mirroredAcl2Handle_;
  Label2LabelActionMap label2LabelActions_;
  std::unique_ptr<BcmWarmBootState> bcmWarmBootState_;
  QosMapKey2QosMapId qosMapKey2QosMapId_;
  QosMapId2QosMap qosMapId2QosMap_;
  UdfGroupNameToInfoMap udfGroupNameToInfoMap_;
  UdfGroupNameToPacketMatcherMap udfGroupNameToPacketMatcherMap_;
  UdfPktMatcherNameToInfoMap udfPktMatcherNameToInfoMap_;

  cfg::L2LearningMode l2LearningMode_;
  std::optional<bool> ptpTcEnabled_;
  bool udfEnabled_{false};

  RouteCounterIDMap routeCounterIDs_;
  int routeCounterModeId_{-1};
  int v6FlexCounterAction_{0};
  int teFlowDstIpPrefixLength_{0};
  int teFlowHintId_{0};
  int teFlowGroupId_{0};
  int teFlowFlexCounterId_{0};
};

} // namespace facebook::fboss
