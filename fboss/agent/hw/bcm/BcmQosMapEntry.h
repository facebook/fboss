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

#include "fboss/agent/hw/bcm/types.h"

#include "fboss/agent/FbossError.h"

extern "C" {
#include <bcm/qos.h>
#include <bcm/types.h>
}

namespace facebook::fboss {

class BcmQosMap;

class BcmQosMapEntry {
 public:
  enum Type { DSCP, EXP };
  BcmQosMapEntry(
      const BcmQosMap& map,
      Type type,
      uint16_t internalTrafficClass,
      uint8_t externalTrafficClass);
  BcmQosMapEntry(const BcmQosMap& map, Type type, const bcm_qos_map_t& entry);
  ~BcmQosMapEntry();

  uint16_t getInternalTrafficClass() const {
    return entry_.int_pri;
  }
  uint8_t getExternalTrafficClass() const {
    switch (type_) {
      case DSCP:
        return entry_.dscp;
      case EXP:
        return entry_.exp;
    }

    throw FbossError("Unknown qos map entry type", type_);
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmQosMapEntry(const BcmQosMapEntry&) = delete;
  BcmQosMapEntry& operator=(const BcmQosMapEntry&) = delete;

  const BcmQosMap& map_;
  Type type_;
  bcm_qos_map_t entry_;
};

} // namespace facebook::fboss
