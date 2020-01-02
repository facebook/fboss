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

/*
 * Stubbed out.
 */
namespace facebook::fboss {

void BcmMacTable::programMacEntry(
    const MacEntry* /*macEntry*/,
    VlanID /*vlan*/) {}
void BcmMacTable::unprogramMacEntry(
    const MacEntry* /*macEntry*/,
    VlanID /*vlan*/) {}

} // namespace facebook::fboss
