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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"

namespace facebook { namespace fboss {

using folly::IPAddress;
using folly::MacAddress;

bool BcmEgress::alreadyExists(const opennsl_l3_egress_t& newEgress) const {
  if (id_ == INVALID) {
    return false;
  }
  opennsl_l3_egress_t existingEgress;
  auto rv = opennsl_l3_egress_get(hw_->getUnit(), id_, &existingEgress);
  bcmCheckError(rv, "Egress object ", id_, " does not exist");
  bool sameMacs = (!newEgress.mac_addr && !existingEgress.mac_addr) ||
    (existingEgress.mac_addr && newEgress.mac_addr &&
     !memcmp(newEgress.mac_addr, existingEgress.mac_addr,
       sizeof(newEgress.mac_addr)));
   return sameMacs && existingEgress.intf == newEgress.intf &&
   existingEgress.port == newEgress.port &&
   existingEgress.flags == newEgress.flags;
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
    sal_memcpy(&eObj.mac_addr, mac->bytes(), sizeof(eObj.mac_addr));
    eObj.port = port;
  }
  eObj.intf = intfId;
  uint32_t flags;
  bool addEgress = false;
  const auto warmBootCache = hw_->getWarmBootCache();
  auto vrfAndIP2EgressCitr = warmBootCache->findEgress(vrf, ip);
  if (vrfAndIP2EgressCitr != warmBootCache->vrfAndIP2Egress_end()) {
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
    const auto& existingEgressIdAndEgress = vrfAndIP2EgressCitr->second;
    // Cache existing egress id
    id_ = existingEgressIdAndEgress.first;
    if (!equivalent(eObj, existingEgressIdAndEgress.second)) {
      VLOG(1) << "Updating egress object for next hop : " << ip;
      addEgress = true;
    } else {
      VLOG(1) << "Egress object for : " << ip << " already exists";
    }
  } else {
    addEgress = true;
  }
  if (addEgress) {
    if (vrfAndIP2EgressCitr == warmBootCache->vrfAndIP2Egress_end()) {
      VLOG(1) << "Adding egress object for next hop : " << ip;
    }
    if (id_ != INVALID) {
      flags |= OPENNSL_L3_REPLACE|OPENNSL_L3_WITH_ID;
    } else {
      flags = 0;
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
          (mac) ? mac->toString() : "ToCPU",
          " on unit ", hw_->getUnit());
      VLOG(3) << "programmed L3 egress object " << id_ << " for "
        << ((mac) ? mac->toString() : "to CPU")
        << " on unit " << hw_->getUnit();
    } else {
      VLOG(1) << "Identical egress object for : " << ip << " pointing to "
        << (mac ? mac->toString() : "CPU ") << " already exists "
        << "skipping egress programming ";
    }
  }
  if (vrfAndIP2EgressCitr != warmBootCache->vrfAndIP2Egress_end()) {
    warmBootCache->programmed(vrfAndIP2EgressCitr);
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
  uint32_t flags;
  if (id_ != INVALID) {
    flags |= OPENNSL_L3_REPLACE|OPENNSL_L3_WITH_ID;
  } else {
    flags = 0;
  }
  auto rc = opennsl_l3_egress_create(hw_->getUnit(), flags, &eObj, &id_);
  bcmCheckError(rc, "failed to program L3 egress object ", id_,
                " to CPU on unit ", hw_->getUnit());
  VLOG(3) << "programmed L3 egress object " << id_
          << " to CPU on unit " << hw_->getUnit();
}

BcmEgress::~BcmEgress() {
  if (id_ == INVALID) {
    return;
  }
  auto rc = opennsl_l3_egress_destroy(hw_->getUnit(), id_);
  bcmLogFatal(rc, hw_, "failed to destroy L3 egress object ",
      id_, " on unit ", hw_->getUnit());
  VLOG(3) << "destroyed L3 egress object " << id_
          << " on unit " << hw_->getUnit();
}

void BcmEcmpEgress::program(opennsl_if_t paths[], int n_path) {
  opennsl_l3_egress_ecmp_t obj;
  opennsl_l3_egress_ecmp_t_init(&obj);
  obj.max_paths = ((n_path + 3) >> 2) << 2; // multiple of 4

  const auto warmBootCache = hw_->getWarmBootCache();
  const auto egressIds = BcmWarmBootCache::toEgressIds(paths, n_path);
  auto egressIds2EcmpCItr = warmBootCache->findEcmp(egressIds);
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
    VLOG(1) << "Ecmp egress object for egress : " <<
      BcmWarmBootCache::toEgressIdsStr(egressIds)
      << " already exists ";
    warmBootCache->programmed(egressIds2EcmpCItr);
  } else {
    VLOG(1) << "Adding ecmp egress with egress : " <<
      BcmWarmBootCache::toEgressIdsStr(egressIds);
    if (id_ != INVALID) {
      obj.flags |= OPENNSL_L3_REPLACE|OPENNSL_L3_WITH_ID;
      obj.ecmp_intf = id_;
    }
    auto ret = opennsl_l3_egress_ecmp_create(hw_->getUnit(), &obj, n_path,
                                             paths);
    bcmCheckError(ret, "failed to program L3 ECMP egress object ", id_,
                " with ", n_path, " paths");
    id_ = obj.ecmp_intf;
    VLOG(3) << "Programmed L3 ECMP egress object " << id_ << " for "
          << n_path << " paths";
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
  VLOG(3) << "Destroyed L3 ECMP egress object " << id_
          << " on unit " << hw_->getUnit();
}

}}
