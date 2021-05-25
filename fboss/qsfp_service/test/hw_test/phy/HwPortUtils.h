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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class PlatformMapping;
class PhyManager;

namespace phy {
class ExternalPhy;
} // namespace phy

namespace utility {
// Verify PhyPortConfig
void verifyPhyPortConfig(
    PortID portID,
    cfg::PortProfileID profileID,
    const PlatformMapping* platformMapping,
    phy::ExternalPhy* xphy);

void verifyPhyPortConnector(PortID portID, PhyManager* xphyManager);
} // namespace utility

} // namespace facebook::fboss
