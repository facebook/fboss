/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/L2Entry.h"

#include <sstream>

namespace facebook::fboss {

L2Entry::L2Entry(
    folly::MacAddress mac,
    VlanID vlan,
    PortDescriptor portDescr,
    L2EntryType type,
    std::optional<cfg::AclLookupClass> classID)
    : mac_(mac),
      vlanID_(vlan),
      portDescr_(portDescr),
      type_(type),
      classID_(classID) {}

std::string L2Entry::str() const {
  std::ostringstream os;

  auto classIDStr = getClassID().has_value()
      ? folly::to<std::string>(static_cast<int>(getClassID().value()))
      : "None";

  os << "L2Entry:: MAC: " << getMac().toString() << " vid: " << getVlanID()
     << " " << getPort().str()
     << " type: " << (static_cast<int>(type_) == 0 ? "PENDING" : "VALIDATED")
     << " classID: " << classIDStr;

  return os.str();
}

} // namespace facebook::fboss
