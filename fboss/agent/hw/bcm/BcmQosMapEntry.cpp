/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmQosMapEntry.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmQosMap.h"
#include "fboss/agent/hw/bcm/BcmQosPolicy.h"

#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPortQueueManager.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook::fboss {

BcmQosMapEntry::BcmQosMapEntry(
    const BcmQosMap& map,
    Type type,
    const bcm_qos_map_t& entry)
    : map_(map), type_(type), entry_(entry) {}

BcmQosMapEntry::BcmQosMapEntry(
    const BcmQosMap& map,
    Type type,
    uint16_t internalTrafficClass,
    uint8_t externalTrafficClass)
    : map_(map), type_(type) {
  bcm_qos_map_t_init(&entry_);
  switch (type) {
    case Type::EXP:
      entry_.exp = externalTrafficClass;
      break;
    case Type::DSCP:
      entry_.dscp = externalTrafficClass;
      break;
  }
  entry_.int_pri = internalTrafficClass;
  auto rv = bcm_qos_map_add(
      map_.getUnit(), map_.getFlags(), &entry_, map_.getHandle());
  bcmCheckError(rv, "failed to add entry in qos map=", map_.getHandle());
}

BcmQosMapEntry::~BcmQosMapEntry() {
  auto rv = bcm_qos_map_delete(
      map_.getUnit(), map_.getFlags(), &entry_, map_.getHandle());
  bcmCheckError(rv, "failed to delete entry from qos map=", map_.getHandle());
}

} // namespace facebook::fboss
