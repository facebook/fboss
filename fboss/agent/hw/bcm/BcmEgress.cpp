/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmEgress.h"

#include "fboss/agent/Constants.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"

#include <folly/logging/xlog.h>
#include <string>

namespace facebook { namespace fboss {

using folly::IPAddress;
using folly::MacAddress;

bool operator==(
    const opennsl_l3_egress_t& lhs,
    const opennsl_l3_egress_t& rhs) {
  bool sameMacs = !memcmp(lhs.mac_addr, rhs.mac_addr, sizeof(lhs.mac_addr));
  bool lhsTrunk = lhs.flags & OPENNSL_L3_TGID;
  bool rhsTrunk = rhs.flags & OPENNSL_L3_TGID;
  bool sameTrunks = lhsTrunk && rhsTrunk && lhs.trunk == rhs.trunk;
  bool samePhysicalPorts = !lhsTrunk && !rhsTrunk && rhs.port == lhs.port;
  bool samePorts = sameTrunks || samePhysicalPorts;
  return sameMacs && samePorts && rhs.intf == lhs.intf &&
      rhs.flags == lhs.flags;
}

bool BcmEgress::alreadyExists(const opennsl_l3_egress_t& newEgress) const {
  if (id_ == INVALID) {
    return false;
  }
  opennsl_l3_egress_t existingEgress;
  auto rv = opennsl_l3_egress_get(hw_->getUnit(), id_, &existingEgress);
  bcmCheckError(rv, "Egress object ", id_, " does not exist");
  return newEgress == existingEgress;
}

void BcmEgress::verifyDropEgress(int unit) {
  const auto id = getDropEgressId();
  opennsl_l3_egress_t egress;
  opennsl_l3_egress_t_init(&egress);
  auto ret = opennsl_l3_egress_get(unit, id, &egress);
  bcmCheckError(ret, "failed to verify drop egress ", id);
  if (!(egress.flags & OPENNSL_L3_DST_DISCARD)) {
    throw FbossError("Egress ID ", id, " is not programmed as drop");
  }
}

void BcmEgress::program(opennsl_if_t intfId, opennsl_vrf_t vrf,
    const IPAddress& ip, const MacAddress* mac, opennsl_port_t port,
    RouteForwardAction action) {
  opennsl_l3_egress_t eObj;
  opennsl_l3_egress_t_init(&eObj);
  if (mac == nullptr) {
    if (action == TO_CPU) {
      eObj.flags |= (OPENNSL_L3_L2TOCPU | OPENNSL_L3_COPY_TO_CPU);
    } else {
      eObj.flags |= OPENNSL_L3_DST_DISCARD;
    }
  } else {
    memcpy(&eObj.mac_addr, mac->bytes(), sizeof(eObj.mac_addr));
    eObj.port = port;
  }
  eObj.intf = intfId;
  bool addOrUpdateEgress = false;
  const auto warmBootCache = hw_->getWarmBootCache();
  CHECK(warmBootCache);
  auto egressId2EgressCitr = warmBootCache->findEgressFromHost(vrf, ip, intfId);
  if (egressId2EgressCitr != warmBootCache->egressId2Egress_end()) {
    // Lambda to compare with existing egress to know if should reprogram
    auto equivalent = [] (const opennsl_l3_egress_t& newEgress,
        const opennsl_l3_egress_t& existingEgress) {
      auto puntToCPU = [=] (const opennsl_l3_egress_t& egress) {
        // Check if both OPENNSL_L3_L2TOCPU and OPENNSL_L3_COPY_TO_CPU are set
        static const auto kCPUFlags =
          OPENNSL_L3_L2TOCPU | OPENNSL_L3_COPY_TO_CPU;
        return (egress.flags & kCPUFlags) == kCPUFlags;
      };
      if (!puntToCPU(newEgress) && !puntToCPU(existingEgress)) {
        // Both new and existing egress point to a valid nexthop
        // Compare mac, port and interface of egress objects
        return !memcmp(newEgress.mac_addr, existingEgress.mac_addr,
          sizeof(newEgress.mac_addr)) &&
          existingEgress.intf == newEgress.intf &&
          existingEgress.port == newEgress.port;
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
      flags |= OPENNSL_L3_REPLACE|OPENNSL_L3_WITH_ID;
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
      auto rc = opennsl_l3_egress_create(hw_->getUnit(), flags, &eObj, &id_);
      bcmCheckError(rc, "failed to program L3 egress object ", id_,
          " ", (mac) ? mac->toString() : "ToCPU",
          " on port ", port,
          " on unit ", hw_->getUnit());
      XLOG(DBG3) << "programmed L3 egress object " << id_ << " for "
                 << ((mac) ? mac->toString() : "to CPU") << " on unit "
                 << hw_->getUnit() << " for ip: " << ip << " @ brcmif "
                 << intfId << " flags " << eObj.flags << " towards port "
                 << eObj.port;
    } else {
      // This could happen when neighbor entry is confirmed with the same MAC
      // after warmboot, as it will trigger another egress programming with the
      // same MAC.
      XLOG(DBG1) << "Identical egress object for : " << ip << " @ brcmif "
                 << intfId << " pointing to "
                 << (mac ? mac->toString() : "CPU ") << " already exists "
                 << "skipping egress programming ";
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
  opennsl_l3_egress_t eObj;
  opennsl_l3_egress_t_init(&eObj);
  eObj.flags |= (OPENNSL_L3_L2TOCPU | OPENNSL_L3_COPY_TO_CPU);
  // BCM does not care about interface ID for punt to CPU egress object
  uint32_t flags = 0;
  if (id_ != INVALID) {
    flags |= OPENNSL_L3_REPLACE|OPENNSL_L3_WITH_ID;
  }
  auto rc = opennsl_l3_egress_create(hw_->getUnit(), flags, &eObj, &id_);
  bcmCheckError(rc, "failed to program L3 egress object ", id_,
                " to CPU on unit ", hw_->getUnit());
  XLOG(DBG3) << "programmed L3 egress object " << id_ << " to CPU on unit "
             << hw_->getUnit();
}

BcmEgress::~BcmEgress() {
  if (id_ == INVALID) {
    return;
  }
  auto rc = opennsl_l3_egress_destroy(hw_->getUnit(), id_);
  bcmLogFatal(rc, hw_, "failed to destroy L3 egress object ",
      id_, " on unit ", hw_->getUnit());
  XLOG(DBG3) << "destroyed L3 egress object " << id_ << " on unit "
             << hw_->getUnit();
}

BcmEcmpEgress::BcmEcmpEgress(const BcmSwitchIf* hw, const Paths& paths)
    : BcmEgressBase(hw), paths_(paths) {
  program();
}

void BcmEcmpEgress::program() {
  opennsl_l3_egress_ecmp_t obj;
  opennsl_l3_egress_ecmp_t_init(&obj);
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
      obj.flags |= OPENNSL_L3_REPLACE|OPENNSL_L3_WITH_ID;
      obj.ecmp_intf = id_;
    }
    opennsl_if_t pathsArray[paths_.size()];
    auto index = 0;
    for (const auto& path : paths_) {
      if (hw_->getHostTable()->isResolved(path)) {
        pathsArray[index++] = path;
      } else {
        XLOG(DBG1) << "Skipping unresolved egress : " << path << " while "
                   << "programming ECMP group ";
      }
    }
    auto ret = opennsl_l3_egress_ecmp_create(hw_->getUnit(), &obj, index,
                                             pathsArray);
    bcmCheckError(ret, "failed to program L3 ECMP egress object ", id_,
                " with ", paths_.size(), " paths");
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
  opennsl_l3_egress_ecmp_t obj;
  opennsl_l3_egress_ecmp_t_init(&obj);
  obj.ecmp_intf = id_;
  auto ret = opennsl_l3_egress_ecmp_destroy(hw_->getUnit(), &obj);
  bcmLogFatal(ret, hw_, "failed to destroy L3 ECMP egress object ",
      id_, " on unit ", hw_->getUnit());
  XLOG(DBG3) << "Destroyed L3 ECMP egress object " << id_ << " on unit "
             << hw_->getUnit();
}

folly::dynamic BcmEgress::toFollyDynamic() const {
  folly::dynamic egress = folly::dynamic::object;
  egress[kEgressId] = getID();
  egress[kMac] = mac_.toString();
  egress[kIntfId] = intfId_;
  return egress;
}

folly::dynamic BcmEcmpEgress::toFollyDynamic() const {
  folly::dynamic ecmpEgress = folly::dynamic::object;
  ecmpEgress[kEgressId] = getID();
  folly::dynamic paths = folly::dynamic::array;
  for (const auto& path : paths_) {
    paths.push_back(path);
  }
  ecmpEgress[kPaths] = std::move(paths);
  return ecmpEgress;
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

void BcmEgress::programToTrunk(opennsl_if_t intfId, opennsl_vrf_t /* vrf */,
                               const folly::IPAddress& /* ip */,
                               const MacAddress mac, opennsl_trunk_t trunk) {
  opennsl_l3_egress_t egress;
  opennsl_l3_egress_t_init(&egress);
  egress.intf = intfId;
  egress.flags |= OPENNSL_L3_TGID;
  egress.trunk = trunk;
  memcpy(egress.mac_addr, mac.bytes(), sizeof(egress.mac_addr));

  uint32_t creationFlags = 0;
  if (id_ != INVALID) {
    creationFlags |= OPENNSL_L3_REPLACE|OPENNSL_L3_WITH_ID;
  }

  if (!alreadyExists(egress)) {
    auto rc = opennsl_l3_egress_create(hw_->getUnit(), creationFlags, &egress,
                                       &id_);

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
    XLOG(DBG3) << "programmed " << desc;
  }

  intfId_ = intfId;
  mac_ = mac;

  CHECK_NE(id_, INVALID);
}

}}
