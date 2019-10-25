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

#include <map>
extern "C" {
#include <opennsl/port.h>
}

namespace facebook {
namespace fboss {

using PortSpeed2TransmitterTechAndMode = std::
    map<cfg::PortSpeed, std::map<TransmitterTechnology, opennsl_port_if_t>>;

const PortSpeed2TransmitterTechAndMode& getSpeedToTransmitterTechAndMode();

} // namespace fboss
} // namespace facebook
