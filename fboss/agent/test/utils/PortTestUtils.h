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
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

namespace facebook::fboss {

class TestEnsembleIf;
struct PortID;

namespace utility {

cfg::PortSpeed getSpeed(cfg::PortProfileID profile);
TransmitterTechnology getMediaType(cfg::PortProfileID profile);
cfg::PortSpeed getDefaultFabricSpeed(const cfg::AsicType& asicType);
cfg::PortSpeed getDefaultInterfaceSpeed(const cfg::AsicType& asicType);
void setCreditWatchdogAndPortTx(
    TestEnsembleIf* ensemble,
    PortID port,
    bool enable);
void enableCreditWatchdog(TestEnsembleIf* ensemble, bool enable);
void setPortTx(TestEnsembleIf* ensemble, PortID port, bool enable);
} // namespace utility
} // namespace facebook::fboss
