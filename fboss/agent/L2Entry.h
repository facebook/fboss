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

#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/Vlan.h"

#include <folly/MacAddress.h>
#include <string>

namespace facebook {
namespace fboss {

enum class L2EntryUpdateType : uint8_t {
  L2_ENTRY_UPDATE_TYPE_DELETE = 0,
  L2_ENTRY_UPDATE_TYPE_ADD,
};

/*
 * L2 entry represents an entry in the hardware L2 table
 */
class L2Entry {
 public:
  L2Entry(folly::MacAddress mac, VlanID vlan, PortDescriptor portDescr);

  const folly::MacAddress& getMac() const {
    return mac_;
  }

  VlanID getVlanID() const {
    return vlanID_;
  }

  PortDescriptor getPortDescriptor() const {
    return portDescr_;
  }

  std::string str() const;

 private:
  folly::MacAddress mac_;
  VlanID vlanID_;
  PortDescriptor portDescr_;
};

} // namespace fboss
} // namespace facebook
