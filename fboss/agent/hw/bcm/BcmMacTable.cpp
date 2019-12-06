/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmMacTable.h"

namespace facebook {
namespace fboss {

void BcmMacTable::processMacAdded(const MacEntry* addedEntry, VlanID vlan) {
  CHECK(addedEntry);
  programMacEntry(addedEntry, vlan);
}

void BcmMacTable::processMacRemoved(const MacEntry* removedEntry, VlanID vlan) {
  CHECK(removedEntry);
  unprogramMacEntry(removedEntry, vlan);
}

void BcmMacTable::processMacChanged(
    const MacEntry* oldEntry,
    const MacEntry* newEntry,
    VlanID vlan) {
  CHECK(oldEntry);
  CHECK(newEntry);

  /*
   * BCN SDK alllows REPLACE with ADD API, and hence it is safe to program the
   * new MAC+vlan directly.
   */
  programMacEntry(newEntry, vlan);
}
} // namespace fboss
} // namespace facebook
