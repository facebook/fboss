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

#include <fboss/agent/gen-cpp2/switch_state_types.h>
#include <folly/MacAddress.h>
#include "fboss/agent/state/MacEntry.h"
#include "fboss/agent/state/NodeMap.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/types.h"

#include <folly/MacAddress.h>
#include <string>

namespace facebook::fboss {

using MacTableTraitsLegacy = NodeMapTraits<folly::MacAddress, MacEntry>;

using MacTableTypeClass = apache::thrift::type_class::map<
    apache::thrift::type_class::string,
    apache::thrift::type_class::structure>;
using MacTableThriftType = std::map<std::string, state::MacEntryFields>;

class MacTable;
using MacTableTraits = ThriftMapNodeTraits<
    MacTable,
    MacTableTypeClass,
    MacTableThriftType,
    MacEntry>;

class MacTable : public ThriftMapNode<MacTable, MacTableTraits> {
 public:
  using Base = ThriftMapNode<MacTable, MacTableTraits>;
  using Base::modify;
  MacTable();
  ~MacTable() override;

  std::shared_ptr<MacEntry> getMacIf(const folly::MacAddress& mac) const {
    return getNodeIf(mac.toString());
  }

  MacTable* modify(Vlan** vlan, std::shared_ptr<SwitchState>* state);
  MacTable* modify(VlanID vlanID, std::shared_ptr<SwitchState>* state);

  void addEntry(const std::shared_ptr<MacEntry>& macEntry) {
    CHECK(!isPublished());
    addNode(macEntry);
  }

  void removeEntry(const folly::MacAddress& mac) {
    CHECK(!isPublished());
    removeNode(mac.toString());
  }

  void updateEntry(
      folly::MacAddress mac,
      PortDescriptor portDescr,
      std::optional<cfg::AclLookupClass> classID,
      std::optional<MacEntryType> type = std::nullopt);

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};

using MacTableDelta = thrift_cow::ThriftMapDelta<MacTable>;

} // namespace facebook::fboss
