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
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class HwQsfpEnsemble;

namespace phy {
class ExternalPhy;
struct PhyPortConfig;
} // namespace phy

namespace utility {
// Verify PhyPortConfig
void verifyPhyPortConfig(
    PortID portID,
    phy::ExternalPhy* xphy,
    const phy::PhyPortConfig& expectedConfig);

void verifyPhyPortConnector(PortID portID, HwQsfpEnsemble* qsfpEnsemble);
std::optional<TransceiverID> getTranscieverIdx(
    PortID portId,
    const HwQsfpEnsemble* ensemble);
PortStatus getPortStatus(PortID portId, const HwQsfpEnsemble* ensemble);
} // namespace utility

} // namespace facebook::fboss
