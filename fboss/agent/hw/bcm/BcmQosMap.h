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

namespace facebook {
namespace fboss {

class BcmSwitchIf;
class BcmQosMapEntry;

class BcmQosMap {
 public:
  explicit BcmQosMap(const BcmSwitchIf* hw);
  BcmQosMap(const BcmSwitchIf* hw, int flags, int mapHandle);
  ~BcmQosMap();

  void clear();
  void addRule(const QosRule& qosRule);
  void removeRule(const QosRule& qosRule);
  bool ruleExists(const QosRule& qosRule) const;
  bool rulesMatch(const std::set<QosRule>& qosRules) const;
  size_t size() const;

  int getUnit() const;
  int getFlags() const;
  int getHandle() const;

 private:
  // Forbidden copy constructor and assignment operator
  BcmQosMap(const BcmQosMap&) = delete;
  BcmQosMap& operator=(const BcmQosMap&) = delete;

  const BcmSwitchIf* hw_;
  int flags_;
  int handle_;
  std::map<QosRule, std::unique_ptr<BcmQosMapEntry>> entries_;
};

} // namespace fboss
} // namespace facebook
