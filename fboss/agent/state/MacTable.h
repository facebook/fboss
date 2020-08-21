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

#include "fboss/agent/state/MacEntry.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>

namespace facebook::fboss {

using MacTableTraits = NodeMapTraits<folly::MacAddress, MacEntry>;

class MacTable : public NodeMapT<MacTable, MacTableTraits> {
 public:
  MacTable();
  ~MacTable() override;

  std::shared_ptr<MacEntry> getMacIf(const folly::MacAddress& mac) const {
    return getNodeIf(mac);
  }

  MacTable* modify(Vlan** vlan, std::shared_ptr<SwitchState>* state);
  MacTable* modify(VlanID vlanID, std::shared_ptr<SwitchState>* state);

  void addEntry(const std::shared_ptr<MacEntry>& macEntry) {
    CHECK(!isPublished());
    addNode(macEntry);
  }

  void removeEntry(const folly::MacAddress& mac) {
    CHECK(!isPublished());
    removeNode(mac);
  }

  void updateEntry(
      folly::MacAddress mac,
      PortDescriptor portDescr,
      std::optional<cfg::AclLookupClass> classID,
      std::optional<MacEntryType> type = std::nullopt);

 private:
  // Inherit the constructors required for clone()
  using NodeMapT::NodeMapT;
  friend class CloneAllocator;
};

using MacTableDelta = NodeMapDelta<MacTable>;

} // namespace facebook::fboss
