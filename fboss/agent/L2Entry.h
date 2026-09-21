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

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/Vlan.h"

#include <folly/MacAddress.h>
#include <string>

namespace facebook::fboss {

std::string l2EntryUpdateTypeStr(L2EntryUpdateType updateType);
/*
 * L2 entry represents an entry in the hardware L2 table
 */
class L2Entry {
 public:
  /*
   * PENDING L2 entries:
   *  - Packets with destination matching PENDING entry are treated as unknown
   *    unicast and flooded.
   *  - If COPY_TO_CPU is enabled for packets with unknown source MAC/vlan,
   *    a PENDING entry does not result into COPY_TO_CPU. This is useful to
   *    avoid flurry of COPY_TO_CPU when a new flow starts: first packet causes
   *    PENDING entry to be learned, and COPY_TO_CPU callback to be triggered,
   *    while subsequent packets don't generate this callback.
   */
  enum class L2EntryType : uint8_t {
    L2_ENTRY_TYPE_PENDING = 0,
    L2_ENTRY_TYPE_VALIDATED,
  };

  L2Entry(
      folly::MacAddress mac,
      VlanID vlan,
      PortDescriptor portDescr,
      L2EntryType type,
      std::optional<cfg::AclLookupClass> classID = std::nullopt);

  const folly::MacAddress& getMac() const {
    return mac_;
  }

  VlanID getVlanID() const {
    return vlanID_;
  }

  PortDescriptor getPort() const {
    return portDescr_;
  }

  L2EntryType getType() const {
    return type_;
  }

  std::optional<cfg::AclLookupClass> getClassID() const {
    return classID_;
  }

  std::string str() const;

 private:
  folly::MacAddress mac_;
  VlanID vlanID_;
  PortDescriptor portDescr_;
  L2EntryType type_;
  std::optional<cfg::AclLookupClass> classID_{std::nullopt};
};

} // namespace facebook::fboss
