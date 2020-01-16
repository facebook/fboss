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
#include "fboss/agent/hw/bcm/BcmEgressManager.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include <folly/logging/xlog.h>
#include <string>

extern "C" {
#include <bcm/l3.h>
}

namespace facebook::fboss {

using folly::IPAddress;
using folly::MacAddress;

bool BcmEgress::alreadyExists(const bcm_l3_egress_t& newEgress) const {
  if (id_ == INVALID) {
    return false;
  }
  bcm_l3_egress_t existingEgress;
  auto rv = bcm_l3_egress_get(hw_->getUnit(), id_, &existingEgress);
  bcmCheckError(rv, "Egress object ", id_, " does not exist");
  return newEgress == existingEgress;
}

void BcmEgress::verifyDropEgress(int unit) {
  const auto id = getDropEgressId();
  bcm_l3_egress_t egress;
  bcm_l3_egress_t_init(&egress);
  auto ret = bcm_l3_egress_get(unit, id, &egress);
  bcmCheckError(ret, "failed to verify drop egress ", id);
  if (!(egress.flags & BCM_L3_DST_DISCARD)) {
    throw FbossError("Egress ID ", id, " is not programmed as drop");
  }
}

void BcmEgress::program(
    bcm_if_t intfId,
    bcm_vrf_t vrf,
    const IPAddress& ip,
    const MacAddress* mac,
    bcm_port_t port,
    RouteForwardAction action) {
  bcm_l3_egress_t eObj;
  auto failedToProgramMsg = folly::to<std::string>(
      "failed to program L3 egress object ",
      id_,
      " ",
      (mac) ? mac->toString() : "ToCPU",
      " on port ",
      port,
      " on unit ",
      hw_->getUnit());
  auto succeededToProgramMsg = folly::to<std::string>(
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
  auto alreadyExistsMsg = folly::to<std::string>(
      "Identical egress object for : ",
      ip,
      " @ brcmif ",
      intfId,
      " pointing to ",
      (mac ? mac->toString() : "CPU "),
      " skipping egress programming");

  if (hasLabel()) {
    auto withLabelMsg = folly::to<std::string>(" with label ", getLabel(), " ");
    failedToProgramMsg.append(withLabelMsg);
    succeededToProgramMsg.append(withLabelMsg);
    alreadyExistsMsg.append(withLabelMsg);
  }

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
      bcmCheckError(rc, failedToProgramMsg);
      XLOG(DBG2) << succeededToProgramMsg << " flags " << eObj.flags
                 << " towards port " << eObj.port;
    } else {
      // This could happen when neighbor entry is confirmed with the same MAC
      // after warmboot, as it will trigger another egress programming with the
      // same MAC.
      XLOG(DBG1) << alreadyExistsMsg;
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

void BcmEgress::prepareEgressObject(
    bcm_if_t intfId,
    bcm_port_t port,
    const std::optional<folly::MacAddress>& mac,
    RouteForwardAction action,
    bcm_l3_egress_t* eObj) const {
  CHECK(eObj);
  bcm_l3_egress_t_init(eObj);
  if (!mac) {
    if (action == TO_CPU) {
      eObj->flags |= (BCM_L3_L2TOCPU | BCM_L3_COPY_TO_CPU);
    } else {
      eObj->flags |= BCM_L3_DST_DISCARD;
    }
  } else {
    memcpy(&(eObj->mac_addr), mac->bytes(), sizeof(eObj->mac_addr));
    eObj->port = port;
  }
  eObj->intf = intfId;
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

BcmEcmpEgress::BcmEcmpEgress(const BcmSwitchIf* hw, const Paths& paths)
    : BcmEgressBase(hw), paths_(paths) {
  program();
}

void BcmEcmpEgress::program() {
  bcm_l3_egress_ecmp_t obj;
  bcm_l3_egress_ecmp_t_init(&obj);
  obj.max_paths = ((paths_.size() + 3) >> 2) << 2; // multiple of 4

  const auto warmBootCache = hw_->getWarmBootCache();
  auto egressIds2EcmpCItr = warmBootCache->findEcmp(paths_);
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
    XLOG(DBG1) << "Ecmp egress object for egress : "
               << BcmWarmBootCache::toEgressIdsStr(paths_)
               << " already exists ";
    warmBootCache->programmed(egressIds2EcmpCItr);
  } else {
    XLOG(DBG1) << "Adding ecmp egress with egress : "
               << BcmWarmBootCache::toEgressIdsStr(paths_);
    if (id_ != INVALID) {
      obj.flags |= BCM_L3_REPLACE | BCM_L3_WITH_ID;
      obj.ecmp_intf = id_;
    }
    bcm_if_t pathsArray[paths_.size()];
    auto index = 0;
    for (const auto& path : paths_) {
      if (hw_->getEgressManager()->isResolved(path)) {
        pathsArray[index++] = path;
      } else {
        XLOG(DBG1) << "Skipping unresolved egress : " << path << " while "
                   << "programming ECMP group ";
      }
    }

    XLOG(DBG2) << "Programming L3 ECMP egress object "
               << ((id_ != INVALID) ? folly::to<std::string>(id_)
                                    : "(invalid id)")
               << " for " << paths_.size() << " paths"
               << ((obj.flags & BCM_L3_REPLACE) ? " replace" : " noreplace")
               << ((obj.flags & BCM_L3_WITH_ID) ? " with id" : " without id");
    auto ret =
        bcm_l3_egress_ecmp_create(hw_->getUnit(), &obj, index, pathsArray);
    bcmCheckError(
        ret,
        "failed to program L3 ECMP egress object ",
        id_,
        " with ",
        paths_.size(),
        " paths");
    id_ = obj.ecmp_intf;
    XLOG(DBG2) << "Programmed L3 ECMP egress object " << id_ << " for "
               << paths_.size() << " paths";
  }
  CHECK_NE(id_, INVALID);
}

BcmEcmpEgress::~BcmEcmpEgress() {
  if (id_ == INVALID) {
    return;
  }
  bcm_l3_egress_ecmp_t obj;
  bcm_l3_egress_ecmp_t_init(&obj);
  obj.ecmp_intf = id_;
  auto ret = bcm_l3_egress_ecmp_destroy(hw_->getUnit(), &obj);
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
  return removeEgressIdHwLocked(hw_->getUnit(), getID(), path);
}

bool BcmEcmpEgress::pathReachableHwLocked(EgressId path) {
  return addEgressIdHwLocked(hw_->getUnit(), getID(), paths_, path);
}

bool BcmEcmpEgress::removeEgressIdHwLocked(
    int unit,
    EgressId ecmpId,
    EgressId toRemove) {
  // We don't really need the lock here so safe to call the
  // stricter non locked version.
  return removeEgressIdHwNotLocked(unit, ecmpId, toRemove);
}

void BcmEgress::programToTrunk(
    bcm_if_t intfId,
    bcm_vrf_t /* vrf */,
    const folly::IPAddress& /* ip */,
    const MacAddress mac,
    bcm_trunk_t trunk) {
  bcm_l3_egress_t egress;
  bcm_l3_egress_t_init(&egress);
  egress.intf = intfId;
  egress.flags |= BCM_L3_TGID;
  egress.trunk = trunk;
  memcpy(egress.mac_addr, mac.bytes(), sizeof(egress.mac_addr));

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

  intfId_ = intfId;
  mac_ = mac;

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
  return sameMacs && samePorts && rhs.intf == lhs.intf &&
      rhs.flags == lhs.flags && lhs.mpls_label == rhs.mpls_label;
}

bool BcmEcmpEgress::removeEgressIdHwNotLocked(
    int unit,
    EgressId ecmpId,
    EgressId toRemove) {
  bcm_l3_egress_ecmp_t obj;
  bcm_l3_egress_ecmp_t_init(&obj);
  obj.ecmp_intf = ecmpId;
  auto ret = bcm_l3_egress_ecmp_delete(unit, &obj, toRemove);
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
    XLOG(FATAL) << "Error removing " << toRemove << " from: " << ecmpId << " "
                << bcm_errmsg(ret);
    return false;
  }
  if (ret != BCM_E_NOT_FOUND) {
    XLOG(DBG1) << " Removed  " << toRemove << " from: " << ecmpId;
  }
  return true;
}

bool BcmEcmpEgress::addEgressIdHwLocked(
    int unit,
    EgressId ecmpId,
    const Paths& pathsInSw,
    EgressId toAdd) {
  if (pathsInSw.find(toAdd) == pathsInSw.end()) {
    // Egress id is not part of this ecmp group. Nothing
    // to do.
    return false;
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
  // checking for presence of egressId to be added and then adding that. Indeed
  // we could get a removeEgressIdHwNotLocked event which removes the concerned
  // egressId just after we checked that the egressId is present. This is
  // actually safe removeEgressIdHwNotLocked is only called from the link scan
  // handler when ports go down. On port going down, we remove the concerned
  // egress entries, mark ARP/NDP for those entries as pending. Then on port
  // up, ARP/NDP will resolve and we will get a second chance to add the egress
  // ID back.
  bcm_l3_egress_ecmp_t existing;
  bcm_l3_egress_ecmp_t_init(&existing);
  existing.ecmp_intf = ecmpId;

  bcm_if_t pathsInHw[pathsInSw.size()];
  int totalPathsInHw;
  auto ret = bcm_l3_egress_ecmp_get(
      unit, &existing, pathsInSw.size(), pathsInHw, &totalPathsInHw);
  bcmCheckError(ret, "Unable to get ecmp entry ", ecmpId);
  int countInHw = 0;
  for (size_t i = 0; i < totalPathsInHw; ++i) {
    if (toAdd == pathsInHw[i]) {
      ++countInHw;
    }
  }
  auto countInSw = pathsInSw.count(toAdd);
  if (countInSw <= countInHw) {
    return false; // Already exists no need to update
  }
  for (int i = 0; i < countInSw - countInHw; ++i) {
    // Egress id exists in s/w but not in HW, add it
    bcm_l3_egress_ecmp_t obj;
    bcm_l3_egress_ecmp_t_init(&obj);
    obj.ecmp_intf = ecmpId;
    ret = bcm_l3_egress_ecmp_add(unit, &obj, toAdd);
    bcmCheckError(ret, "Error adding ", toAdd, " to ", ecmpId);
    XLOG(DBG1) << "Added " << toAdd << " to " << ecmpId;
  }
  return true;
}

bcm_mpls_label_t getLabel(const bcm_l3_egress_t& egress) {
  return reinterpret_cast<const bcm_l3_egress_t&>(egress).mpls_label;
}

} // namespace facebook::fboss
