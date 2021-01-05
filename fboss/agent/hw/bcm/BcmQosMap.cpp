/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmQosMap.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPortQueueManager.h"
#include "fboss/agent/hw/bcm/BcmQosMapEntry.h"
#include "fboss/agent/hw/bcm/BcmQosPolicy.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/QosPolicy.h"

extern "C" {
#include <bcm/qos.h>
}

namespace {
using namespace facebook::fboss;
constexpr auto kQosMapIngressL3Flags = BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3;

BcmQosMap::Type getType(int flags) {
  if ((flags & (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3)) ==
      (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3)) {
    return BcmQosMap::IP_INGRESS;
  }
  if ((flags & (BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_L3)) ==
      (BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_L3)) {
    return BcmQosMap::IP_EGRESS;
  }
  if ((flags & (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_MPLS)) ==
      (BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_MPLS)) {
    return BcmQosMap::MPLS_INGRESS;
  }
  if ((flags & (BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_MPLS)) ==
      (BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_MPLS)) {
    return BcmQosMap::MPLS_EGRESS;
  }
  throw FbossError("Invalid flags for qos map");
}

BcmQosMapEntry::Type getQosMapEntryType(BcmQosMap::Type type) {
  switch (type) {
    case BcmQosMap::IP_INGRESS:
    case BcmQosMap::IP_EGRESS:
      return BcmQosMapEntry::Type::DSCP;
    case BcmQosMap::MPLS_INGRESS:
    case BcmQosMap::MPLS_EGRESS:
      return BcmQosMapEntry::Type::EXP;
  }

  throw FbossError("Unknown QoS type", type);
}

} // namespace

namespace facebook::fboss {

size_t BcmQosMap::size() const {
  return entries_.size();
}

int BcmQosMap::getFlags() const {
  return flags_;
}

int BcmQosMap::getUnit() const {
  return hw_->getUnit();
}

int BcmQosMap::getHandle() const {
  return handle_;
}

BcmQosMap::BcmQosMap(const BcmSwitchIf* hw, Type type)
    : hw_(hw), type_(type), flags_(getQosMapFlags(type)) {
  auto rv = bcm_qos_map_create(hw_->getUnit(), flags_, &handle_);
  bcmCheckError(rv, "failed to create qos map");
}

BcmQosMap::BcmQosMap(const BcmSwitchIf* hw, int flags, int mapHandle)
    : hw_(hw), type_(::getType(flags)), flags_(flags), handle_(mapHandle) {
  int numEntries = 0;
  auto rv = bcm_qos_map_multi_get(
      hw_->getUnit(), flags, handle_, 0, nullptr, &numEntries);
  bcmCheckError(rv, "failed to get the number of element in qos map=", handle_);
  std::vector<bcm_qos_map_t> bcmEntries(numEntries);
  rv = bcm_qos_map_multi_get(
      hw_->getUnit(),
      flags,
      handle_,
      bcmEntries.size(),
      bcmEntries.data(),
      &numEntries);
  bcmCheckError(rv, "failed to get the entries of qos map=", handle_);
  for (auto i = 0; i < numEntries; i++) {
    auto& bcmEntry = bcmEntries[i];
    if (bcmEntry.color != bcmColorGreen || bcmEntry.int_pri > BCM_PRIO_MAX) {
      /* Egress QoS Map returns entries even for other colors. ignore those
       * entries since we only care about "green" entries and did not program
       * entries of other colors. */
      continue;
    }
    XLOG(DBG4) << "found entry [color: " << bcmEntry.color
               << ", int_pri: " << bcmEntry.int_pri
               << ", exp]: " << bcmEntry.exp << "for qos map: " << handle_;
    entries_.insert(std::make_unique<BcmQosMapEntry>(
        *this, getQosMapEntryType(type_), bcmEntry));
  }
}

void BcmQosMap::addRule(
    uint16_t internalTrafficClass,
    uint8_t externalTrafficClass) {
  /*
   * We must explicitly configure QoS map for default queue (queue 0) or else,
   * the packets will be processed by default queue as expected, but the dscp
   * marking will be removed (dscp set to 0). Note that this behavior is
   * different from ACLs where dscp marking is not removed if no ACL is
   * configured for default queue.
   * CS7475492 raises this question with Broadcom, but in the interim, unlike
   * previously, we don't skip creating QoS map for queue0.
   */
  /* TODO(pshaikh): Remove Qos rule altogether and use only qos maps */
  entries_.insert(std::make_unique<BcmQosMapEntry>(
      *this,
      getQosMapEntryType(type_),
      internalTrafficClass,
      externalTrafficClass));
}

void BcmQosMap::clear() {
  entries_.clear();
}

void BcmQosMap::removeRule(
    uint16_t internalTrafficClass,
    uint8_t externalTrafficClass) {
  for (auto itr = entries_.begin(); itr != entries_.end(); itr++) {
    if ((*itr)->getInternalTrafficClass() == internalTrafficClass &&
        (*itr)->getExternalTrafficClass() == externalTrafficClass) {
      itr = entries_.erase(itr);
      break;
    }
  }
}

bool BcmQosMap::ruleExists(
    uint16_t internalTrafficClass,
    uint8_t externalTrafficClass) const {
  for (auto itr = entries_.cbegin(); itr != entries_.cend(); itr++) {
    if ((*itr)->getInternalTrafficClass() == internalTrafficClass &&
        (*itr)->getExternalTrafficClass() == externalTrafficClass) {
      return true;
    }
  }
  return false;
}

BcmQosMap::~BcmQosMap() {
  entries_.clear();
  auto rv = bcm_qos_map_destroy(hw_->getUnit(), handle_);
  bcmCheckError(rv, "failed to destroy qos map=", handle_);
}

int BcmQosMap::getQosMapFlags(BcmQosMap::Type type) {
  switch (type) {
    case BcmQosMap::IP_INGRESS:
      return BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_L3;

    case BcmQosMap::IP_EGRESS:
      return BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_L3;

    case BcmQosMap::MPLS_INGRESS:
      return BCM_QOS_MAP_INGRESS | BCM_QOS_MAP_MPLS;

    case BcmQosMap::MPLS_EGRESS:
      return BCM_QOS_MAP_EGRESS | BCM_QOS_MAP_MPLS;
  }
  throw FbossError("Unknown qos map type");
}

} // namespace facebook::fboss
