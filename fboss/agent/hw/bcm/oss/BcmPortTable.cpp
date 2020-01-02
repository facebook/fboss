/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmPortTable.h"

namespace facebook::fboss {

// stubbed out
void BcmPortTable::initPortGroups() {}

void BcmPortTable::initPortGroupLegacy(BcmPort* /* controllingPort */) {}

void BcmPortTable::initPortGroupFromConfig(
    BcmPort* /* controllingPort */,
    const std::map<PortID, std::vector<PortID>>& /* subsidiaryPortsMap */) {}

} // namespace facebook::fboss
