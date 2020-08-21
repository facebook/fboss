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

#include "fboss/agent/L2Entry.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

class SwitchState;
class Vlan;

class MacTableUtils {
 public:
  static std::shared_ptr<SwitchState> updateMacTable(
      const std::shared_ptr<SwitchState>& state,
      L2Entry l2Entry,
      L2EntryUpdateType l2EntryUpdateType);

  static std::shared_ptr<SwitchState> updateOrAddEntryWithClassID(
      const std::shared_ptr<SwitchState>& state,
      VlanID vlanID,
      const std::shared_ptr<MacEntry>& macEntry,
      cfg::AclLookupClass classID);

  static std::shared_ptr<SwitchState> removeClassIDForEntry(
      const std::shared_ptr<SwitchState>& state,
      VlanID vlanID,
      const std::shared_ptr<MacEntry>& macEntry);

  static std::shared_ptr<SwitchState> updateOrAddStaticEntry(
      const std::shared_ptr<SwitchState>& state,
      const PortDescriptor& port,
      VlanID vlan,
      folly::MacAddress mac);

  static std::shared_ptr<SwitchState> removeEntry(
      const std::shared_ptr<SwitchState>& state,
      VlanID vlan,
      folly::MacAddress mac);

  static std::shared_ptr<SwitchState> updateOrAddStaticEntryIfNbrExists(
      const std::shared_ptr<SwitchState>& state,
      VlanID vlan,
      folly::MacAddress mac);
};

} // namespace facebook::fboss
