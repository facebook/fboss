/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmEgress.h"

#include "fboss/agent/Constants.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/bcm/BcmEcmpUtils.h"
#include "fboss/agent/hw/bcm/BcmEgressManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

#include <folly/logging/xlog.h>
#include <string>

extern "C" {
#include <bcm/l3.h>
}

// TODO Remove this hard coding of Dlb Max ECMP Id
// once SDK API is available, query the SDK and set this during the init.
namespace {
constexpr auto kDlbEcmpMaxId = 200128;
}

namespace facebook::fboss {

using folly::IPAddress;
using folly::MacAddress;

static bool isFlowletPortAttributesSupported(const BcmSwitchIf* hw) {
  return (
      (hw->getPlatform()->getAsic()->isSupported(HwAsic::Feature::FLOWLET)) &&
      (hw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::FLOWLET_PORT_ATTRIBUTES)));
}

bool BcmEgress::alreadyExists(const bcm_l3_egress_t& newEgress) const {
  if (id_ == INVALID) {
    return false;
  }
  bcm_l3_egress_t existingEgress;
  auto rv = bcm_l3_egress_get(hw_->getUnit(), id_, &existingEgress);
  bcmCheckError(rv, "Egress object ", id_, " does not exist");
  return newEgress == existingEgress;
}

void BcmEgress::setupDefaultDropEgress(int unit, bcm_if_t dropEgressID) {
  bcm_l3_egress_t egress;
  bcm_l3_egress_t_init(&egress);
  auto rv = bcm_l3_egress_get(unit, dropEgressID, &egress);
  bcmCheckError(rv, "failed to get drop egress ", dropEgressID);
  if (!(egress.flags & BCM_L3_DST_DISCARD)) {
    throw FbossError("Egress ID ", dropEgressID, " is not programmed as drop");
  }
  XLOG(DBG1) << "Default drop egress: " << dropEgressID
             << " is already created";
}

void BcmEgress::program(
    bcm_if_t intfId,
    bcm_vrf_t vrf,
    const IPAddress& ip,
    const MacAddress* mac,
    bcm_port_t port,
    RouteForwardAction action) {
  bcm_l3_egress_t eObj;
  auto failedToProgramMsg = [=]() {
    auto msg = folly::to<std::string>(
        "failed to program L3 egress object ",
        id_,
        " ",
        (mac) ? mac->toString() : "ToCPU",
        " on port ",
        port,
        " on unit ",
        hw_->getUnit());
    if (hasLabel()) {
      auto withLabelMsg =
          folly::to<std::string>(" with label ", getLabel(), " ");
      msg.append(withLabelMsg);
    }
    return msg;
  };
  auto succeededToProgramMsg = [=]() {
    auto msg = folly::to<std::string>(
        "programmed L3 egress object ",
        id_,
        " for ",
        ((mac) ? mac->toString() : "to CPU"),
        " on unit ",
        hw_->getUnit(),
        " for ip: ",
        ip,
        " @ brcmif ",
        intfId);
    if (hasLabel()) {
      auto withLabelMsg =
          folly::to<std::string>(" with label ", getLabel(), " ");
      msg.append(withLabelMsg);
    }
    return msg;
  };
  auto alreadyExistsMsg = [=]() {
    auto msg = folly::to<std::string>(
        "Identical egress object for : ",
        ip,
        " @ brcmif ",
        intfId,
        " pointing to ",
        (mac ? mac->toString() : "CPU "),
        " skipping egress programming");
    if (hasLabel()) {
      auto withLabelMsg =
          folly::to<std::string>(" with label ", getLabel(), " ");
      msg.append(withLabelMsg);
    }
    return msg;
  };

  prepareEgressObject(
      intfId,
      port,
      !mac ? std::nullopt : std::make_optional<folly::MacAddress>(*mac),
      action,
      &eObj);

  bool addOrUpdateEgress = false;
  const auto warmBootCache = hw_->getWarmBootCache();
  CHECK(warmBootCache);
  // TODO(pshaikh) : look for labeled egress in warmboot cache
  auto egressId2EgressCitr = findEgress(vrf, intfId, ip);
  if (egressId2EgressCitr != warmBootCache->egressId2Egress_end()) {
    // Lambda to compare with existing egress to know if should reprogram
    auto equivalent = [](const bcm_l3_egress_t& newEgress,
                         const bcm_l3_egress_t& existingEgress) {
      auto puntToCPU = [=](const bcm_l3_egress_t& egress) {
        // Check if both BCM_L3_L2TOCPU and BCM_L3_COPY_TO_CPU are set
        static const auto kCPUFlags = BCM_L3_L2TOCPU | BCM_L3_COPY_TO_CPU;
        return (egress.flags & kCPUFlags) == kCPUFlags;
      };
      if (!puntToCPU(newEgress) && !puntToCPU(existingEgress)) {
        // Both new and existing egress point to a valid nexthop
        // Compare mac, port and interface of egress objects
        return !memcmp(
                   newEgress.mac_addr,
                   existingEgress.mac_addr,
                   sizeof(newEgress.mac_addr)) &&
            existingEgress.intf == newEgress.intf &&
            existingEgress.port == newEgress.port &&
            existingEgress.dynamic_scaling_factor ==
            newEgress.dynamic_scaling_factor &&
            existingEgress.dynamic_load_weight ==
            newEgress.dynamic_load_weight &&
            existingEgress.dynamic_queue_size_weight ==
            newEgress.dynamic_queue_size_weight &&
            facebook::fboss::getLabel(existingEgress) ==
            facebook::fboss::getLabel(newEgress);
      }
      if (puntToCPU(existingEgress)) {
        // If existing entry and new entry both point to CPU we
        // consider them equal (i.e. no need to update h/w)
        return puntToCPU(newEgress);
      } else {
        // Existing entry does not point to CPU while the new
        // one does. This is because the new entry does not know
        // the result of ARP/NDP resolution yet. Leave the entry
        // in h/w intact
        return true;
      }
      return false;
    };
    const auto& existingEgressId = egressId2EgressCitr->first;
    // Cache existing egress id
    id_ = existingEgressId;
    auto existingEgressObject = egressId2EgressCitr->second;
    if (!equivalent(eObj, existingEgressObject)) {
      XLOG(DBG1) << "Updating egress object for next hop : " << ip
                 << " @ brcmif " << intfId;
      addOrUpdateEgress = true;
    } else {
      XLOG(DBG1) << "Egress object for: " << ip << " @ brcmif " << intfId
                 << " already exists";
    }
  } else {
    addOrUpdateEgress = true;
  }
  if (addOrUpdateEgress) {
    uint32_t flags = 0;
    if (id_ != INVALID) {
      flags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
    }
    if (!alreadyExists(eObj)) {
      /*
       *  Only program the HW if a identical egress object does not
       *  exist. Per BCM documentation updating entries like so should not
       *  be a problem. However we found that if we reprogrammed a already
       *  existing egress object with the same parameters forwarding to
       *  the corresponding IP address sometimes broke. BCM issue is being
       *  tracked in t4324084
       */
      auto rc = bcm_l3_egress_create(hw_->getUnit(), flags, &eObj, &id_);
      bcmCheckError(rc, failedToProgramMsg());
      XLOG(DBG2) << succeededToProgramMsg() << " flags " << eObj.flags
                 << " towards port " << eObj.port;
    } else {
      // This could happen when neighbor entry is confirmed with the same MAC
      // after warmboot, as it will trigger another egress programming with the
      // same MAC.
      XLOG(DBG1) << alreadyExistsMsg();
    }
  }
  // update our internal fields
  mac_ = (mac != nullptr) ? *mac : folly::MacAddress{};
  intfId_ = intfId;

  // update warmboot cache if needed
  if (egressId2EgressCitr != warmBootCache->egressId2Egress_end()) {
    warmBootCache->programmed(egressId2EgressCitr);
  }
  CHECK_NE(id_, INVALID);
}

void BcmEgress::programToCPU() {
  auto egressFromCache = hw_->getWarmBootCache()->getToCPUEgressId();
  if (id_ == INVALID && egressFromCache != INVALID) {
    id_ = egressFromCache;
    return;
  }
  bcm_l3_egress_t eObj;
  bcm_l3_egress_t_init(&eObj);
  eObj.flags |= (BCM_L3_L2TOCPU | BCM_L3_COPY_TO_CPU);
  // BCM does not care about interface ID for punt to CPU egress object
  uint32_t flags = 0;
  if (id_ != INVALID) {
    flags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
  }
  auto rc = bcm_l3_egress_create(hw_->getUnit(), flags, &eObj, &id_);
  bcmCheckError(
      rc,
      "failed to program L3 egress object ",
      id_,
      " to CPU on unit ",
      hw_->getUnit());
  XLOG(DBG2) << "programmed L3 egress object " << id_ << " to CPU on unit "
             << hw_->getUnit();
}

void BcmEgress::updateFlowletConfig(bcm_l3_egress_t& eObj, bcm_port_t port)
    const {
  auto* bcmPort = hw_->getPortTable()->getBcmPortIf(port);
  bcm_gport_t gport = BcmPort::asGPort(port);
  if (bcmPort && (gport != BCM_GPORT_LOCAL_CPU)) {
    auto bcmFlowletConfig = bcmPort->getPortFlowletConfig();
    if (*bcmFlowletConfig.scalingFactor()) {
      eObj.dynamic_scaling_factor = *bcmFlowletConfig.scalingFactor();
    }
    if (*bcmFlowletConfig.loadWeight()) {
      eObj.dynamic_load_weight = *bcmFlowletConfig.loadWeight();
    }
    if (*bcmFlowletConfig.queueWeight()) {
      eObj.dynamic_queue_size_weight = *bcmFlowletConfig.queueWeight();
    }
    XLOG(DBG2) << "Programmed ScalingFactor=" << eObj.dynamic_scaling_factor
               << " LoadWeight=" << eObj.dynamic_load_weight
               << " QueueWeight=" << eObj.dynamic_queue_size_weight;
  }
}

void BcmEgress::prepareEgressObject(
    bcm_if_t intfId,
    bcm_port_t port,
    const std::optional<folly::MacAddress>& mac,
    RouteForwardAction action,
    bcm_l3_egress_t* eObj) const {
  CHECK(eObj);
  bcm_l3_egress_t_init(eObj);
  if (!mac) {
    if (action == RouteForwardAction::TO_CPU) {
      eObj->flags |= (BCM_L3_L2TOCPU | BCM_L3_COPY_TO_CPU);
    } else {
      eObj->flags |= BCM_L3_DST_DISCARD;
    }
  } else {
    memcpy(&(eObj->mac_addr), mac->bytes(), sizeof(eObj->mac_addr));
    eObj->port = port;
  }
  eObj->intf = intfId;
  // Update Flowlet config on the egress object
  // For TH3 Port Flowlet configs are programmed in egress object
  // For TH4 Port Flowlet configs are programmed in port object
  if (!isFlowletPortAttributesSupported(hw_)) {
    XLOG(DBG2) << "Updating Egress object for flowlet:  " << id_;
    updateFlowletConfig(*eObj, port);
  }
}

// TODO Ideally this function should check on the portFlowletConfig_ in BcmPort
// instead of checking in egress object.That check would suffice for both TH3
// and TH4.
bool BcmEgress::isFlowletEnabled(int unit, bcm_if_t id) {
  bcm_l3_egress_t obj;
  bcm_l3_egress_t_init(&obj);
  auto rv = bcm_l3_egress_get(unit, id, &obj);
  bcmCheckError(rv, "failed to get egress object", id);
  if ((obj.dynamic_load_weight == BCM_L3_ECMP_DYNAMIC_SCALING_FACTOR_INVALID) ||
      (obj.dynamic_queue_size_weight ==
       BCM_L3_ECMP_DYNAMIC_LOAD_WEIGHT_INVALID) ||
      (obj.dynamic_scaling_factor ==
       BCM_L3_ECMP_DYNAMIC_QUEUE_SIZE_WEIGHT_INVALID)) {
    XLOG(DBG2) << "Egress is not programmed with flowlet config: " << id
               << " Port " << obj.port;
    return false;
  }
  return true;
}

BcmWarmBootCache::EgressId2EgressCitr BcmEgress::findEgress(
    bcm_vrf_t vrf,
    bcm_if_t intfId,
    const folly::IPAddress& ip) const {
  return hw_->getWarmBootCache()->findEgressFromHost(vrf, ip, intfId);
}

BcmEgress::~BcmEgress() {
  if (id_ == INVALID) {
    return;
  }
  auto rc = bcm_l3_egress_destroy(hw_->getUnit(), id_);
  bcmLogFatal(
      rc,
      hw_,
      "failed to destroy L3 egress object ",
      id_,
      " on unit ",
      hw_->getUnit());
  XLOG(DBG2) << "destroyed L3 egress object " << id_ << " on unit "
             << hw_->getUnit();
}

BcmEcmpEgress::BcmEcmpEgress(
    const BcmSwitchIf* hw,
    const EgressId2Weight& egressId2Weight,
    const bool isUcmp)
    : BcmEgressBase(hw), egressId2Weight_(egressId2Weight) {
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::WEIGHTED_NEXTHOPGROUP_MEMBER)) {
    ucmpEnabled_ = isUcmp;
  }
  if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::HSDK)) {
    useHsdk_ = true;
  }
  if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::WIDE_ECMP)) {
    wideEcmpSupported_ = true;
  }
  program();
}

void BcmEcmpEgress::setEgressEcmpMemberStatus(
    const BcmSwitchIf* hw,
    const EgressId2Weight& egressId2Weight) {
  for (const auto& path : egressId2Weight) {
    if (path.second && hw->getEgressManager()->isResolved(path.first)) {
      XLOG(DBG2) << "Enabling DLB ecmp member on hw " << path.first;
      auto rc = bcm_l3_egress_ecmp_member_status_set(
          hw->getUnit(), path.first, BCM_L3_ECMP_DYNAMIC_MEMBER_HW);
      bcmCheckError(rc, "Failed to enable DLB on egress member ", path.first);
    } else {
      XLOG(DBG1) << "Skipping unresolved or zero weight egress : " << path.first
                 << " weight : " << path.second << " while enabling DLB ";
    }
  }
}

bool BcmEcmpEgress::isFlowletEnabledOnAllEgress(
    const EgressId2Weight& egressId2Weight) {
  for (const auto& path : egressId2Weight) {
    if (!BcmEgress::isFlowletEnabled(hw_->getUnit(), path.first)) {
      return false;
    }
  }
  return true;
}

int BcmEcmpEgress::getEcmpObject(
    bcm_l3_egress_ecmp_t* obj,
    int* pathsInHwCount,
    bcm_l3_ecmp_member_t* membersInHw,
    bcm_if_t* pathsInHw) {
  bcm_l3_egress_ecmp_t_init(obj);
  obj->ecmp_intf = id_;
  int ret = 0;

  // Initialize oftherwise SDK may return junk
  memset(membersInHw, 0, sizeof(bcm_l3_ecmp_member_t) * kMaxWeightedEcmpPaths);
  if (useHsdk_) {
    ret = bcm_l3_ecmp_get(
        hw_->getUnit(),
        obj,
        kMaxWeightedEcmpPaths,
        membersInHw,
        pathsInHwCount);
  } else {
    ret = bcm_l3_egress_ecmp_get(
        hw_->getUnit(), obj, kMaxWeightedEcmpPaths, pathsInHw, pathsInHwCount);
  }

  return ret;
}

EcmpDetails BcmEcmpEgress::getEcmpDetails() {
  bcm_l3_egress_ecmp_t obj;
  int pathsInHwCount = -1;
  bcm_l3_ecmp_member_t membersInHw[kMaxWeightedEcmpPaths];
  bcm_if_t pathsInHw[kMaxWeightedEcmpPaths];
  EcmpDetails ecmpDetails;

  int ret = getEcmpObject(&obj, &pathsInHwCount, membersInHw, pathsInHw);
  bcmCheckError(ret, "Unable to get ECMP:  ", id_);
  ecmpDetails.ecmpId() = id_;
  ecmpDetails.flowletEnabled() =
      ((obj.dynamic_mode == BCM_L3_ECMP_DYNAMIC_MODE_NORMAL ||
        (obj.dynamic_mode == BCM_L3_ECMP_DYNAMIC_MODE_OPTIMAL))
           ? true
           : false);
  ecmpDetails.flowletInterval() = obj.dynamic_age;
  ecmpDetails.flowletTableSize() = obj.dynamic_size;
  return ecmpDetails;
}

bool BcmEcmpEgress::isFlowletConfigUpdateNeeded() {
  bcm_l3_egress_ecmp_t obj;
  int pathsInHwCount = -1;
  bcm_l3_ecmp_member_t membersInHw[kMaxWeightedEcmpPaths];
  bcm_if_t pathsInHw[kMaxWeightedEcmpPaths];
  bool updateNeeded = false;

  int ret = getEcmpObject(&obj, &pathsInHwCount, membersInHw, pathsInHw);
  bcmCheckError(ret, "Unable to get ECMP:  ", id_);
  auto bcmEcmpFlowletConfig = hw_->getEgressManager()->getBcmFlowletConfig();

  const auto configDynamicMode =
      utility::getFlowletDynamicMode(bcmEcmpFlowletConfig.switchingMode);
  // if required and current modes match, no updated needed
  if (obj.dynamic_mode != configDynamicMode ||
      obj.dynamic_size != bcmEcmpFlowletConfig.flowletTableSize) {
    const auto neededDynamicSize = utility::getFlowletSizeWithScalingFactor(
        static_cast<const BcmSwitch*>(hw_),
        bcmEcmpFlowletConfig.flowletTableSize,
        pathsInHwCount,
        bcmEcmpFlowletConfig.maxLinks);
    if ((obj.dynamic_age != bcmEcmpFlowletConfig.inactivityIntervalUsecs) ||
        (obj.dynamic_size != neededDynamicSize)) {
      updateNeeded = true;
    }

    if (neededDynamicSize != 0) {
      updateNeeded = true;
    }
  }

  return updateNeeded;
}

bool BcmEcmpEgress::updateFlowletConfig(
    bcm_l3_egress_ecmp_t& obj,
    int numPaths) {
  bool flowletConfigUpdated = false;
  if (FLAGS_flowletSwitchingEnable) {
    bool egressFlowletEnabled = true;
    // check only on TH3 since egress objet need update on TH3 for flowlet
    // TODO  Remove this check once TH4 port flowlet config check added
    if (!isFlowletPortAttributesSupported(hw_)) {
      egressFlowletEnabled = isFlowletEnabledOnAllEgress(egressId2Weight_);
    }

    if (egressFlowletEnabled) {
      auto bcmFlowletConfig = hw_->getEgressManager()->getBcmFlowletConfig();
      obj.dynamic_age = bcmFlowletConfig.inactivityIntervalUsecs;
      // Check if the id is less than max dlb allowed ecmp id.
      // if not set the dynamic size as 0 for not to enable flowlet on ECMP
      if (id_ >= kDlbEcmpMaxId) {
        XLOG(WARN) << "Flowlet switching not updated due to id limit.ECMP: "
                   << id_;
        obj.dynamic_size = 0;
      } else {
        obj.dynamic_size = utility::getFlowletSizeWithScalingFactor(
            static_cast<const BcmSwitch*>(hw_),
            bcmFlowletConfig.flowletTableSize,
            numPaths,
            bcmFlowletConfig.maxLinks);
      }
      if (obj.dynamic_size > 0) {
        obj.dynamic_mode =
            utility::getFlowletDynamicMode(bcmFlowletConfig.switchingMode);
        flowletConfigUpdated = true;
      }
    }
    XLOG(DBG2) << "Programmed FlowletTableSize=" << obj.dynamic_size
               << " InactivityIntervalUsecs=" << obj.dynamic_age
               << " DynamicMode =" << obj.dynamic_mode << " for ECMP object "
               << ((id_ != INVALID) ? folly::to<std::string>(id_)
                                    : "(invalid id)");
  }
  return flowletConfigUpdated;
}

void BcmEcmpEgress::program() {
  bcm_l3_egress_ecmp_t obj;
  bcm_l3_egress_ecmp_t_init(&obj);
  int numPaths = 0;
  if (ucmpEnabled_) {
    numPaths = egressId2Weight_.size();
  } else {
    for (auto path : egressId2Weight_) {
      numPaths += path.second;
    }
  }
  obj.max_paths = ((numPaths + 3) >> 2) << 2; // multiple of 4
  bool enableFlowletMemberStatus = false;
  bool isExistingEcmp = false;

  const auto warmBootCache = hw_->getWarmBootCache();
  auto egressIds2EcmpCItr = warmBootCache->findEcmp(egressId2Weight_);
  if (egressIds2EcmpCItr != warmBootCache->egressIds2Ecmp_end()) {
    const auto& existing = egressIds2EcmpCItr->second;
    // TODO figure out why the following check fails
    // i.e. the suggested max_paths is not the same
    // as programmed max_paths. Note that this does not
    // imply that less number of next hops got programmed
    // but just that the max_paths did not get rounded to
    // the next multiple of 4 as we desired.
    // CHECK(obj.max_paths == existing.max_paths);
    id_ = existing.ecmp_intf;
    isExistingEcmp = true;
    XLOG(DBG1) << "Ecmp egress object for egress : "
               << BcmWarmBootCache::toEgressId2WeightStr(egressId2Weight_)
               << " already exists ";
    warmBootCache->programmed(egressIds2EcmpCItr);
  } else {
    XLOG(DBG1) << "Adding ecmp egress with egress : "
               << BcmWarmBootCache::toEgressId2WeightStr(egressId2Weight_);
    int option = 0;
    if (id_ != INVALID) {
      obj.flags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
      obj.ecmp_intf = id_;
      option = BCM_L3_ECMP_O_REPLACE | BCM_L3_ECMP_O_CREATE_WITH_ID;
    }

    XLOG(DBG2) << "Programming L3 ECMP egress object "
               << ((id_ != INVALID) ? folly::to<std::string>(id_)
                                    : "(invalid id)")
               << " for " << numPaths << " paths"
               << ((obj.flags & BCM_L3_REPLACE) ? " replace" : " noreplace")
               << ((obj.flags & BCM_L3_WITH_ID) ? " with id" : " without id")
               << ", native ucmp " << (ucmpEnabled_ ? "enabled" : "disabled")
               << ", flowlet switching "
               << (FLAGS_flowletSwitchingEnable ? "enabled" : "disabled");
    int ret = 0;

    // if we know the ECMP Id already, we can update the flowlet config here
    //  and do the max id check.
    isExistingEcmp = (id_ != INVALID) ? true : false;
    if (FLAGS_flowletSwitchingEnable && isExistingEcmp) {
      enableFlowletMemberStatus = updateFlowletConfig(obj, numPaths);
    }

    // @lint-ignore CLANGTIDY
    bcm_l3_ecmp_member_t ecmpMemberArray[numPaths];
    // @lint-ignore CLANGTIDY
    bcm_if_t pathsArray[numPaths];
    auto index = 0;
    auto idx = 0;

    if (useHsdk_) {
      if (ucmpEnabled_) {
        obj.ecmp_group_flags |= BCM_L3_ECMP_MEMBER_WEIGHTED;
      }
      for (const auto& path : egressId2Weight_) {
        if (ucmpEnabled_) {
          if (hw_->getEgressManager()->isResolved(path.first)) {
            bcm_l3_ecmp_member_t_init(&ecmpMemberArray[idx]);
            ecmpMemberArray[idx].egress_if = path.first;
            ecmpMemberArray[idx].weight = path.second;
            idx++;
          } else {
            XLOG(DBG1) << "Skipping unresolved egress : " << path.first
                       << " while programming ECMP group ";
          }
        } else {
          for (int i = 0; i < path.second; i++) {
            if (hw_->getEgressManager()->isResolved(path.first)) {
              bcm_l3_ecmp_member_t_init(&ecmpMemberArray[idx]);
              ecmpMemberArray[idx].egress_if = path.first;
              idx++;
            } else {
              XLOG(DBG1) << "Skipping unresolved egress : " << path.first
                         << " while " << "programming ECMP group ";
            }
          }
        }
      }
      ret = bcm_l3_ecmp_create(
          hw_->getUnit(), option, &obj, idx, ecmpMemberArray);
      if (ret == BCM_E_RESOURCE) {
        XLOG(DBG2)
            << "Got BCM_E_RESOURCE when programming ecmp, replace it by BCM_E_FULL to trigger graceful error handling";
        ret = BCM_E_FULL;
      }
    } else {
      // check whether WideECMP is needed
      if (numPaths > kMaxNonWeightedEcmpPaths) {
        createWideEcmpEntry(numPaths);
        return;
      } else {
        for (const auto& path : egressId2Weight_) {
          for (int i = 0; i < path.second; i++) {
            if (hw_->getEgressManager()->isResolved(path.first)) {
              pathsArray[index++] = path.first;
            } else {
              XLOG(DBG1) << "Skipping unresolved egress : " << path.first
                         << " while " << "programming ECMP group ";
            }
          }
        }
        ret =
            bcm_l3_egress_ecmp_create(hw_->getUnit(), &obj, index, pathsArray);
      }
    }
    bcmCheckError(
        ret,
        "failed to program L3 ECMP egress object ",
        id_,
        " with ",
        numPaths,
        " paths");
    id_ = obj.ecmp_intf;
    XLOG(DBG2) << "Programmed L3 ECMP egress object " << id_ << " for "
               << numPaths << " paths";
    // This is newly created ECMP object, might need flowlet config update
    if (FLAGS_flowletSwitchingEnable && !isExistingEcmp) {
      // if flowlet config is updated then re program it.
      if (updateFlowletConfig(obj, numPaths)) {
        int option = BCM_L3_ECMP_O_REPLACE | BCM_L3_ECMP_O_CREATE_WITH_ID;
        obj.flags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
        if (useHsdk_) {
          ret = bcm_l3_ecmp_create(
              hw_->getUnit(), option, &obj, idx, ecmpMemberArray);
        } else {
          ret = bcm_l3_egress_ecmp_create(
              hw_->getUnit(), &obj, index, pathsArray);
        }
        bcmCheckError(ret, "failed to re-program L3 ECMP egress object ", id_);
        setEgressEcmpMemberStatus(hw_, egressId2Weight_);
      }
    }
  }
  CHECK_NE(id_, INVALID);
  // Enable each ECMP member to be DLB enabled on TH3 and TH4
  if (FLAGS_flowletSwitchingEnable && enableFlowletMemberStatus) {
    setEgressEcmpMemberStatus(hw_, egressId2Weight_);
  }
}

BcmEcmpEgress::~BcmEcmpEgress() {
  if (id_ == INVALID) {
    return;
  }
  int ret;
  if (useHsdk_) {
    ret = bcm_l3_ecmp_destroy(hw_->getUnit(), id_);
  } else {
    bcm_l3_egress_ecmp_t obj;
    bcm_l3_egress_ecmp_t_init(&obj);
    obj.ecmp_intf = id_;
    ret = bcm_l3_egress_ecmp_destroy(hw_->getUnit(), &obj);
  }
  bcmLogFatal(
      ret,
      hw_,
      "failed to destroy L3 ECMP egress object ",
      id_,
      " on unit ",
      hw_->getUnit());
  XLOG(DBG2) << "Destroyed L3 ECMP egress object " << id_ << " on unit "
             << hw_->getUnit();
}

bool BcmEcmpEgress::pathUnreachableHwLocked(EgressId path) {
  return removeEgressIdHwLocked(
      hw_->getUnit(),
      getID(),
      egressId2Weight_,
      path,
      ucmpEnabled_,
      wideEcmpSupported_,
      useHsdk_);
}

bool BcmEcmpEgress::pathReachableHwLocked(EgressId path) {
  return addEgressIdHwLocked(
      hw_,
      hw_->getUnit(),
      getID(),
      egressId2Weight_,
      path,
      hw_->getRunState(),
      ucmpEnabled_,
      wideEcmpSupported_,
      useHsdk_);
}

bool BcmEcmpEgress::removeEgressIdHwLocked(
    int unit,
    EgressId ecmpId,
    const EgressId2Weight& egressIdInSw,
    EgressId toRemove,
    bool ucmpEnabled,
    bool wideEcmpSupported,
    bool useHsdk) {
  auto it = egressIdInSw.find(toRemove);
  if (it == egressIdInSw.end() || it->second == 0) {
    return false;
  }
  // We don't really need the lock here so safe to call the
  // stricter non locked version.
  return removeEgressIdHwNotLocked(
      unit,
      ecmpId,
      std::make_pair(toRemove, it->second),
      ucmpEnabled,
      wideEcmpSupported,
      useHsdk);
}

void BcmEgress::prepareEgressObjectOnTrunk(
    bcm_if_t intfId,
    bcm_trunk_t trunk,
    const folly::MacAddress& mac,
    bcm_l3_egress_t* egress) const {
  bcm_l3_egress_t_init(egress);
  egress->intf = intfId;
  egress->flags |= BCM_L3_TGID;
  egress->trunk = trunk;
  memcpy(egress->mac_addr, mac.bytes(), sizeof(egress->mac_addr));
}

void BcmEgress::programToTrunk(
    bcm_if_t intfId,
    bcm_vrf_t vrf,
    const folly::IPAddress& ip,
    const MacAddress mac,
    bcm_trunk_t trunk) {
  bcm_l3_egress_t egress;
  prepareEgressObjectOnTrunk(intfId, trunk, mac, &egress);

  bool addOrUpdateEgress = false;
  const auto warmBootCache = hw_->getWarmBootCache();
  CHECK(warmBootCache);
  auto egressId2EgressCitr = findEgress(vrf, intfId, ip);
  if (egressId2EgressCitr != warmBootCache->egressId2Egress_end()) {
    auto equivalent = [](const bcm_l3_egress_t& newEgress,
                         const bcm_l3_egress_t& existingEgress) {
      // Compare mac, port and interface of egress objects
      return !memcmp(
                 newEgress.mac_addr,
                 existingEgress.mac_addr,
                 sizeof(newEgress.mac_addr)) &&
          existingEgress.intf == newEgress.intf &&
          existingEgress.port == newEgress.port;
    };
    const auto& existingEgressId = egressId2EgressCitr->first;
    // Cache existing egress id
    id_ = existingEgressId;
    auto existingEgressObject = egressId2EgressCitr->second;
    if (!equivalent(egress, existingEgressObject)) {
      XLOG(DBG1) << "Updating egress object for next hop : " << ip
                 << " @ brcmif " << intfId;
      addOrUpdateEgress = true;
    } else {
      XLOG(DBG1) << "Egress object for: " << ip << " @ brcmif " << intfId
                 << " already exists";
    }
  } else {
    addOrUpdateEgress = true;
  }

  if (addOrUpdateEgress) {
    uint32_t creationFlags = 0;
    if (id_ != INVALID) {
      creationFlags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
    }

    if (!alreadyExists(egress)) {
      auto rc =
          bcm_l3_egress_create(hw_->getUnit(), creationFlags, &egress, &id_);

      auto desc = folly::to<std::string>(
          "L3 egress object ",
          id_,
          " to ",
          mac.toString(),
          " on trunk ",
          trunk,
          " on unit ",
          hw_->getUnit());
      bcmCheckError(rc, "failed to program ", desc);
      XLOG(DBG2) << "programmed " << desc;
    }
  }
  intfId_ = intfId;
  mac_ = mac;

  // update warmboot cache if needed
  if (egressId2EgressCitr != warmBootCache->egressId2Egress_end()) {
    warmBootCache->programmed(egressId2EgressCitr);
  }
  CHECK_NE(id_, INVALID);
}

bool operator==(const bcm_l3_egress_t& lhs, const bcm_l3_egress_t& rhs) {
  bool sameMacs = !memcmp(lhs.mac_addr, rhs.mac_addr, sizeof(lhs.mac_addr));
  bool lhsTrunkPort = lhs.flags & BCM_L3_TGID;
  bool rhsTrunkPort = rhs.flags & BCM_L3_TGID;
  bool sameTrunks = lhsTrunkPort && rhsTrunkPort && lhs.trunk == rhs.trunk;
  bool samePhysicalPorts =
      !lhsTrunkPort && !rhsTrunkPort && rhs.port == lhs.port;
  bool samePorts = sameTrunks || samePhysicalPorts;
  bool sameScalingFactor =
      lhs.dynamic_scaling_factor == rhs.dynamic_scaling_factor;
  bool sameLoadWeight = lhs.dynamic_load_weight == rhs.dynamic_load_weight;
  bool sameQueueWeight =
      lhs.dynamic_queue_size_weight == rhs.dynamic_queue_size_weight;
  return sameMacs && samePorts && sameScalingFactor && sameLoadWeight &&
      sameQueueWeight && rhs.intf == lhs.intf && rhs.flags == lhs.flags &&
      lhs.mpls_label == rhs.mpls_label;
}

bool BcmEcmpEgress::removeEgressIdHwNotLocked(
    int unit,
    EgressId ecmpId,
    std::pair<EgressId, int> toRemove,
    bool ucmpEnabled,
    bool wideEcmpSupported,
    bool useHsdk) {
  int ret = 0;
  if (useHsdk) {
    bcm_l3_ecmp_member_t member;
    bcm_l3_ecmp_member_t_init(&member);
    member.egress_if = toRemove.first;
    if (ucmpEnabled) {
      // This function will always blow away all egress members
      // associated with the toRemove egress intf.
      ret = bcm_l3_ecmp_member_delete(unit, ecmpId, &member);
    } else {
      for (int i = 0; i < toRemove.second; i++) {
        ret = bcm_l3_ecmp_member_delete(unit, ecmpId, &member);
        if (ret) {
          break;
        }
      }
    }
  } else {
    if (isWideEcmpEnabled(wideEcmpSupported) &&
        rebalanceWideEcmpEntry(unit, ecmpId, toRemove)) {
      return true;
    }
    for (int i = 0; i < toRemove.second; i++) {
      XLOG(DBG3) << " Removing  " << toRemove.first << " from: " << ecmpId;
      bcm_l3_egress_ecmp_t obj;
      bcm_l3_egress_ecmp_t_init(&obj);
      obj.ecmp_intf = ecmpId;
      ret = bcm_l3_egress_ecmp_delete(unit, &obj, toRemove.first);
      if (ret) {
        break;
      }
    }
  }
  if (ret && ret != BCM_E_NOT_FOUND) {
    // If ret == BCM_E_NOT_FOUND this means that
    // egress entry was not present in ECMP group (likely
    // Already deleted). This is fine as marking entries
    // as pending/resolved, adding them back etc happens
    // in the update thread whereas removing entries
    // happens in link scan callback itself, we could get into a
    // case where a link goes down, comes back up and then goes
    // down again. In which case the neither
    // a) Marking entries as pending (which breaks the port association)
    // b) adding back of egress entries
    // may not have happened before the second down event.

    // This may get called from linkscan handler which would
    // swallow an exception. So fail hard in case of error
    XLOG(FATAL) << "Error removing " << toRemove.first << " from: " << ecmpId
                << " " << bcm_errmsg(ret);
    return false;
  }
  if (ret != BCM_E_NOT_FOUND) {
    XLOG(DBG1) << " Removed  " << toRemove.first << " from: " << ecmpId
               << " with native ucmp "
               << (ucmpEnabled ? "enabled" : "disabled");
  }
  return true;
}

bool BcmEcmpEgress::addEgressIdHwLocked(
    const BcmSwitchIf* hw,
    int unit,
    EgressId ecmpId,
    const EgressId2Weight& egressId2WeightInSw,
    EgressId toAdd,
    SwitchRunState runState,
    bool ucmpEnabled,
    bool wideEcmpSupported,
    bool useHsdk) {
  if (egressId2WeightInSw.find(toAdd) == egressId2WeightInSw.end()) {
    // Egress id is not part of this ecmp group. Nothing
    // to do.
    return false;
  }
  int numPaths = 0;
  if (ucmpEnabled) {
    numPaths = egressId2WeightInSw.size();
  } else {
    for (auto path : egressId2WeightInSw) {
      numPaths += path.second;
    }
  }

  // We always check for egress Id already existing before adding it to the
  // ECMP group for 2 use cases:
  // a) Port X goes down and we remove Egress object (say) PE from all ecmp
  //    objects. Subsquently we add another ecmp egress object say Y that
  //    includes PE, now when port comes back up we will try to add PE back to
  //    all ecmp egress objects that include it, but Y already has PE and we
  //    will get an error. So check before adding.
  // b) On warm boot host table calls linkUpHwLocked for all ports that are up
  //    (since ports may have come up while we were not running). Thus there
  //    may be ports which were up both before and after, for which we would
  //    try to add egressIds that already are in h/w, hence the check.
  //
  // (a) can be tackled by guarding against routes pointing to unresolved
  // egress entries, but not (b)
  // It might appear that there is a race b/w getting the ECMP entry from HW,
  // checking for presence of egressId to be added and then adding that.
  // Indeed we could get a removeEgressIdHwNotLocked event which removes the
  // concerned egressId just after we checked that the egressId is present.
  // This is actually safe removeEgressIdHwNotLocked is only called from the
  // link scan handler when ports go down. On port going down, we remove the
  // concerned egress entries, mark ARP/NDP for those entries as pending. Then
  // on port up, ARP/NDP will resolve and we will get a second chance to add
  // the egress ID back.
  bcm_l3_egress_ecmp_t existing;
  bcm_l3_egress_ecmp_t_init(&existing);
  existing.ecmp_intf = ecmpId;
  int countInHw = 0;
  int ret;
  // @lint-ignore CLANGTIDY
  bcm_l3_ecmp_member_t membersInHw[numPaths];
  int totalMembersInHw = -1;
  int memberIndex = -1;
  std::set<EgressId> activeMembers;

  if (useHsdk) {
    ret = bcm_l3_ecmp_get(
        unit, &existing, numPaths, membersInHw, &totalMembersInHw);
    bcmCheckError(ret, "Unable to get ecmp entry ", ecmpId);
    for (size_t i = 0; i < totalMembersInHw; ++i) {
      if (toAdd == membersInHw[i].egress_if) {
        if (ucmpEnabled) {
          countInHw = membersInHw[i].weight;
          memberIndex = i;
          break;
        } else {
          ++countInHw;
        }
      }
    }
  } else {
    // @lint-ignore CLANGTIDY
    // In WideECMP case, HW will have kMaxWeightedEcmpPaths entries
    bcm_if_t pathsInHw[kMaxWeightedEcmpPaths];
    int totalPathsInHw;
    ret = bcm_l3_egress_ecmp_get(
        unit, &existing, kMaxWeightedEcmpPaths, pathsInHw, &totalPathsInHw);
    bcmCheckError(ret, "Unable to get ecmp entry ", ecmpId);
    for (size_t i = 0; i < totalPathsInHw; ++i) {
      activeMembers.emplace(pathsInHw[i]);
      if (toAdd == pathsInHw[i]) {
        ++countInHw;
      }
    }
  }
  auto countInSw = egressId2WeightInSw.at(toAdd);
  if (countInSw <= countInHw) {
    return false; // Already exists no need to update
  }

  if (useHsdk) {
    if (ucmpEnabled) {
      if (memberIndex != -1) {
        // member already exists, just update its weight
        membersInHw[memberIndex].weight = countInSw;
        ret = bcm_l3_ecmp_create(
            unit,
            BCM_L3_ECMP_O_CREATE_WITH_ID | BCM_L3_ECMP_O_REPLACE,
            &existing,
            totalMembersInHw,
            membersInHw);
        if (ret == BCM_E_RESOURCE) {
          XLOG(DBG2)
              << "Got BCM_E_RESOURCE when programming ecmp, replace it by BCM_E_FULL to trigger graceful error handling";
          ret = BCM_E_FULL;
        }
        bcmCheckError(
            ret,
            "Error updating weight of member ",
            toAdd,
            " in ecmp entry ",
            ecmpId);
      } else {
        // member does not exist, add it
        bcm_l3_ecmp_member_t member;
        bcm_l3_ecmp_member_t_init(&member);
        member.egress_if = toAdd;
        member.weight = countInSw;
        ret = bcm_l3_ecmp_member_add(unit, ecmpId, &member);
        if (ret == BCM_E_RESOURCE) {
          XLOG(DBG2)
              << "Got BCM_E_RESOURCE when adding ecmp member, replace it by BCM_E_FULL to trigger graceful error handling";
          ret = BCM_E_FULL;
        }
        bcmCheckError(
            ret, "Error adding member ", toAdd, " to ecmp entry ", ecmpId);
      }
      XLOG(DBG1) << "Added " << toAdd << " to " << ecmpId
                 << " with native ucmp enabled";
    } else {
      for (int i = 0; i < countInSw - countInHw; ++i) {
        bcm_l3_ecmp_member_t member;
        bcm_l3_ecmp_member_t_init(&member);
        member.egress_if = toAdd;
        ret = bcm_l3_ecmp_member_add(unit, ecmpId, &member);
        bcmCheckError(ret, "Error adding ", toAdd, " to ", ecmpId);
        if (runState < SwitchRunState::INITIALIZED) {
          XLOG(DBG1) << "Added " << toAdd << " to " << ecmpId
                     << " with native ucmp disabled"
                     << " before transitioning to INIT state";
        } else {
          XLOG(DBG1) << "Added " << toAdd << " to " << ecmpId
                     << " with native ucmp disabled";
        }
      }
    }
  } else {
    // check whether we need to create weighted ecmp
    if (numPaths <= kMaxNonWeightedEcmpPaths) {
      for (int i = 0; i < countInSw - countInHw; ++i) {
        // Egress id exists in s/w but not in HW, add it
        bcm_l3_egress_ecmp_t obj;
        bcm_l3_egress_ecmp_t_init(&obj);
        obj.ecmp_intf = ecmpId;
        ret = bcm_l3_egress_ecmp_add(unit, &obj, toAdd);
        bcmCheckError(ret, "Error adding ", toAdd, " to ", ecmpId);
        if (runState < SwitchRunState::INITIALIZED) {
          // If a port transitioned to down state before warm boot
          // and in the rare scenario that ARP/ND was not removed,
          // the ECMP member will be added back here. However this
          // will be short lived since linkUp/DownHwLocked will be
          // involked at end of warmboot init.
          XLOG(DBG1) << "Added " << toAdd << " to " << ecmpId
                     << " before transitioning to INIT state";
        } else {
          XLOG(DBG1) << "Added " << toAdd << " to " << ecmpId;
        }
      }
    } else {
      if (!isWideEcmpEnabled(wideEcmpSupported)) {
        XLOG(ERR) << "Wide ECMP is not enabled. NumPaths : " << numPaths
                  << " Ecmp Width : " << FLAGS_ecmp_width;
        return false;
      }
      // create weighted ECMP
      activeMembers.emplace(toAdd);
      programWideEcmp(unit, ecmpId, egressId2WeightInSw, activeMembers);
      XLOG(DBG1) << "Added " << toAdd << " to wide ecmp " << ecmpId;
      return true;
    }
  }
  // Enable DLB on ecmp newly added member on TH3 and TH4
  if (FLAGS_flowletSwitchingEnable) {
    // TODO  Remove this check once TH4 port flowlet config check added
    if (!isFlowletPortAttributesSupported(hw) &&
        !BcmEgress::isFlowletEnabled(unit, toAdd)) {
      XLOG(DBG2) << "Not Enabling DLB ecmp member on hw ECMP: " << ecmpId
                 << " as flowlet is not programmed on  egress id: " << toAdd;
      return true;
    }
    XLOG(DBG2) << "Enabling DLB ecmp member on hw " << toAdd;
    ret = bcm_l3_egress_ecmp_member_status_set(
        unit, toAdd, BCM_L3_ECMP_DYNAMIC_MEMBER_HW);
    bcmCheckError(ret, "Failed to enable DLB on egress member ", toAdd);
  }
  return true;
}

void BcmEcmpEgress::programForFlowletSwitching() {
  // Check needed to avoid the post warmboot programming if config is same
  if (isFlowletConfigUpdateNeeded()) {
    XLOG(DBG3) << "Updating flowlet switching config on ECMP " << id_;
    return program();
  }
}

void BcmEcmpEgress::normalizeUcmpToMaxPath(
    const EgressId2Weight& egressId2Weight,
    const std::set<EgressId>& activeMembers,
    const uint32_t normalizedPathCount,
    EgressId2Weight& normalizedEgressId2Weight) {
  std::vector<uint64_t> egressWeights;
  for (const auto& member : activeMembers) {
    egressWeights.emplace_back(egressId2Weight.at(member));
  }
  RouteNextHopEntry::normalizeNextHopWeightsToMaxPaths(
      egressWeights, normalizedPathCount);

  int idx = 0;
  for (const auto& member : activeMembers) {
    normalizedEgressId2Weight.insert(
        std::make_pair(member, egressWeights.at(idx++)));
  }
  XLOG(DBG2) << "Updated weights for WideECMP "
             << egressId2WeightToString(normalizedEgressId2Weight);
}

void BcmEcmpEgress::programWideEcmp(
    int unit,
    bcm_if_t& id,
    const EgressId2Weight& egressId2Weight,
    const std::set<EgressId>& activeMembers) {
  bcm_l3_egress_ecmp_t obj;
  bcm_l3_egress_ecmp_t_init(&obj);
  // weighted path size needs to be power of 2
  // @lint-ignore CLANGTIDY
  bcm_if_t pathsArray[kMaxWeightedEcmpPaths];
  auto index = 0;
  if (id != INVALID) {
    obj.flags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
    obj.ecmp_intf = id;
  }
  // Need at least one member to create weighted path
  if (activeMembers.size()) {
    obj.ecmp_group_flags |= BCM_L3_ECMP_WEIGHTED;
    obj.max_paths = FLAGS_ecmp_width;
    EgressId2Weight normalizedEgressId2WeightInSw;
    normalizeUcmpToMaxPath(
        egressId2Weight,
        activeMembers,
        FLAGS_ecmp_width,
        normalizedEgressId2WeightInSw);
    for (const auto& path : normalizedEgressId2WeightInSw) {
      for (auto i = 0; i < path.second; i++) {
        pathsArray[index++] = path.first;
      }
    }
  } else {
    // No resolved paths - create a normal ECMP with no members
    obj.max_paths = kMaxNonWeightedEcmpPaths;
  }
  auto ret = bcm_l3_egress_ecmp_create(unit, &obj, index, pathsArray);
  bcmCheckError(
      ret,
      "failed to program L3 wide ECMP egress object ",
      id,
      " with ",
      FLAGS_ecmp_width,
      " paths");
  id = obj.ecmp_intf;
  XLOG(DBG2) << "Programmed L3 wide ECMP egress object " << id << " for "
             << FLAGS_ecmp_width << " paths";
  CHECK_NE(id, INVALID);
}

bool BcmEcmpEgress::rebalanceWideEcmpEntry(
    int unit,
    EgressId ecmpId,
    std::pair<EgressId, int> toRemove) {
  // @lint-ignore CLANGTIDY
  bcm_if_t pathsInHw[kMaxWeightedEcmpPaths];
  int totalPathsInHw;
  bcm_l3_egress_ecmp_t obj;
  bcm_l3_egress_ecmp_t_init(&obj);
  obj.ecmp_intf = ecmpId;

  // check the number of members in hw
  int ret = bcm_l3_egress_ecmp_get(
      unit, &obj, kMaxWeightedEcmpPaths, pathsInHw, &totalPathsInHw);
  bcmCheckError(ret, "Unable to get ecmp entry ", ecmpId);

  if (totalPathsInHw > kMaxNonWeightedEcmpPaths) {
    // use hw info to reconstruct weight map and active member list.
    // This gets called on link down from linkscan thread
    // where we don't have access to ECMP Egress object.
    std::map<EgressId, NextHopWeight> weightMap;
    std::set<EgressId> activeMembers;
    for (size_t i = 0; i < totalPathsInHw; ++i) {
      if (pathsInHw[i] != toRemove.first) {
        weightMap[pathsInHw[i]]++;
        activeMembers.emplace(pathsInHw[i]);
      }
    }
    // create ID to weight mapping from membership info
    // read from hardware.
    EgressId2Weight egressId2Weight;
    for (const auto& path : weightMap) {
      egressId2Weight.insert(std::make_pair(path.first, path.second));
    }
    // rebalance with remaining active members
    programWideEcmp(unit, ecmpId, egressId2Weight, activeMembers);
    XLOG(DBG1) << " Removed  " << toRemove.first
               << " from wide ecmp : " << ecmpId;
    return true;
  }
  // not a wide ECMP entry
  return false;
}

void BcmEcmpEgress::createWideEcmpEntry(int numPaths) {
  if (!isWideEcmpEnabled(hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::WIDE_ECMP))) {
    XLOG(ERR) << "Wide ECMP is not supported/enabled. NumPaths : " << numPaths
              << " Ecmp Width : " << FLAGS_ecmp_width;
    return;
  }

  std::set<EgressId> activeMembers;
  for (const auto& path : egressId2Weight_) {
    if (hw_->getEgressManager()->isResolved(path.first)) {
      activeMembers.insert(path.first);
    } else {
      XLOG(DBG1) << "Skipping unresolved egress : " << path.first << " while "
                 << "programming ECMP group ";
    }
  }
  programWideEcmp(hw_->getUnit(), id_, egressId2Weight_, activeMembers);
}

std::string BcmEcmpEgress::egressId2WeightToString(
    const EgressId2Weight& egressId2Weight) {
  std::stringstream ss;
  int idx{0};
  for (const auto& egressId : egressId2Weight) {
    idx++;
    ss << egressId.first;
    ss << "(";
    ss << egressId.second;
    ss << std::string(")") + (idx < egressId2Weight.size() ? ", " : "");
  }
  return ss.str();
}

bcm_mpls_label_t getLabel(const bcm_l3_egress_t& egress) {
  return reinterpret_cast<const bcm_l3_egress_t&>(egress).mpls_label;
}

bool BcmEcmpEgress::isWideEcmpEnabled(bool wideEcmpSupported) {
  return wideEcmpSupported && FLAGS_ecmp_width > kMaxNonWeightedEcmpPaths;
}

bool BcmEcmpEgress::updateEcmpDynamicMode() {
  bcm_l3_egress_ecmp_t obj;
  int pathsInHwCount = -1;
  bcm_l3_ecmp_member_t membersInHw[kMaxWeightedEcmpPaths];
  bcm_if_t pathsInHw[kMaxWeightedEcmpPaths];
  int ret = 0;
  bool updateComplete = true;

  // DLB is supported only on upto first 128 groups from the ecmp dlb offset.
  // This is due to limitation of broadcom hardware resources for DLB.
  // In our case ECMP Ids 200000-200127 only can be ugraded to DLB.
  // Check if the id is less than max dlb allowed ecmp id.
  if (id_ >= kDlbEcmpMaxId) {
    XLOG(WARN) << "Flowlet switching not updated due to id limit.ECMP: " << id_;
    return updateComplete;
  }

  // check only on TH3 since egress objet need update on TH3 for flowlet
  // TODO  Remove this check once TH4 port flowlet config check added
  if (!isFlowletPortAttributesSupported(hw_) &&
      !isFlowletEnabledOnAllEgress(egressId2Weight_)) {
    // Egress is not updated, return updateComplete
    return updateComplete;
  }

  ret = getEcmpObject(&obj, &pathsInHwCount, membersInHw, pathsInHw);
  bcmCheckError(ret, "Unable to get ECMP:  ", id_);

  // get copy of the ecmpFlowletConfig
  auto bcmEcmpFlowletConfig = hw_->getEgressManager()->getBcmFlowletConfig();
  const auto configDynamicMode =
      utility::getFlowletDynamicMode(bcmEcmpFlowletConfig.switchingMode);
  // check if the current dynamic mode is same as configured dynamic mode
  // if it is same nothing to do
  if (obj.dynamic_mode == configDynamicMode) {
    XLOG(DBG3) << "ECMP: " << id_ << " is already in dynamic mode "
               << obj.dynamic_mode << " Skip dynamic mode update.";
    return updateComplete;
  }

  // just ensure we are still running in flowlet mode which is reflected
  // in the cfg cached here.
  if (!bcmEcmpFlowletConfig.inactivityIntervalUsecs) {
    // we don't need any change
    return updateComplete;
  }

  int flowletTableSize = bcmEcmpFlowletConfig.flowletTableSize;
  const auto adjustedFlowletTableSize =
      utility::getFlowletSizeWithScalingFactor(
          static_cast<const BcmSwitch*>(hw_),
          flowletTableSize,
          pathsInHwCount,
          bcmEcmpFlowletConfig.maxLinks);

  const auto adjustedDynamicMode = (adjustedFlowletTableSize > 0)
      ? configDynamicMode
      : BCM_L3_ECMP_DYNAMIC_MODE_DISABLED;
  // if the HW state i.e. dynamic_mode doesn't match the expectation after
  // adjust, lets fix it
  if (adjustedDynamicMode != BCM_L3_ECMP_DYNAMIC_MODE_DISABLED) {
    int option = BCM_L3_ECMP_O_REPLACE | BCM_L3_ECMP_O_CREATE_WITH_ID;
    obj.flags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
    obj.dynamic_size = adjustedFlowletTableSize;
    obj.dynamic_mode = adjustedDynamicMode;
    obj.dynamic_age = bcmEcmpFlowletConfig.inactivityIntervalUsecs;
    obj.ecmp_intf = id_;
    XLOG(DBG2) << "Performing ecmp object adjustment for ECMP: " << id_
               << " , with dynamic size: " << adjustedFlowletTableSize
               << " , with dynamic mode: " << adjustedDynamicMode;

    if (useHsdk_) {
      ret = bcm_l3_ecmp_create(
          hw_->getUnit(), option, &obj, pathsInHwCount, membersInHw);
    } else {
      ret = bcm_l3_egress_ecmp_create(
          hw_->getUnit(), &obj, pathsInHwCount, pathsInHw);
    }
    bcmCheckError(ret, "failed to re-program L3 ECMP egress object ", id_);

    setEgressEcmpMemberStatus(hw_, egressId2Weight_);
  } else {
    // if we didn't adjust for dynamic mode, lets skip
    updateComplete = false;
  }
  return updateComplete;
}

uint64_t BcmEcmpEgress::getL3EcmpDlbFailPackets() {
  uint64_t dlbDropCount = 0;
  // Flowlet switching might not be enabled all the ecmp groups during the init
  // or during lot of link flaps event due to flowset resource scarce.
  // Reading the dlb stats on non flowlet enabled ecmp group will throw SDK
  // error and subsequently crash in bcmCheckError
  auto ecmpDetails = getEcmpDetails();
  if (FLAGS_flowletSwitchingEnable && ecmpDetails.flowletEnabled().value()) {
    XLOG(DBG3) << "Read l3 ecmp dlb stats: " << id_;
    auto rv = bcm_l3_ecmp_dlb_stat_get(
        hw_->getUnit(), id_, bcmL3ECMPDlbStatFailPackets, &dlbDropCount);
    bcmCheckError(rv, "failed to get l3 ecmp dlb stat ", id_);
  }
  return dlbDropCount;
}

} // namespace facebook::fboss
