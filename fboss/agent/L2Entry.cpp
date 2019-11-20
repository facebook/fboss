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

namespace facebook {
namespace fboss {

L2Entry::L2Entry(folly::MacAddress mac, VlanID vlan, PortDescriptor portDescr)
    : mac_(mac), vlanID_(vlan), portDescr_(portDescr) {}

std::string L2Entry::str() const {
  std::ostringstream os;

  os << "L2Entry:: MAC: " << getMac().toString() << " vid: " << getVlanID()
     << " " << getPortDescriptor().str();

  return os.str();
}

} // namespace fboss
} // namespace facebook
