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
#include "fboss/agent/state/QosPolicy.h"

namespace facebook::fboss {

class BcmSwitchIf;
class BcmQosMapEntry;

class BcmQosMap {
 public:
  enum Type { IP_INGRESS, IP_EGRESS, MPLS_INGRESS, MPLS_EGRESS };
  BcmQosMap(const BcmSwitchIf* hw, Type type);
  BcmQosMap(const BcmSwitchIf* hw, int flags, int mapHandle);
  ~BcmQosMap();

  void clear();
  void addRule(uint16_t internalTrafficClass, uint8_t externalTrafficClass);
  void removeRule(uint16_t internalTrafficClass, uint8_t externalTrafficClass);
  bool ruleExists(uint16_t internalTrafficClass, uint8_t externalTrafficClass)
      const;
  size_t size() const;

  int getUnit() const;
  int getFlags() const;
  int getHandle() const;
  Type getType() const {
    return type_;
  }

  static int getQosMapFlags(Type type);

 private:
  // Forbidden copy constructor and assignment operator
  BcmQosMap(const BcmQosMap&) = delete;
  BcmQosMap& operator=(const BcmQosMap&) = delete;

  const BcmSwitchIf* hw_;
  Type type_;
  int flags_;
  int handle_;
  std::set<std::unique_ptr<BcmQosMapEntry>> entries_;
};

} // namespace facebook::fboss
