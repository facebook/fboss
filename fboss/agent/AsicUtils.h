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

#include "fboss/agent/hw/switch_asics/Jericho2Asic.h"
#include "fboss/agent/hw/switch_asics/Jericho3Asic.h"
#include "fboss/agent/hw/switch_asics/Ramon3Asic.h"
#include "fboss/agent/hw/switch_asics/RamonAsic.h"

namespace facebook::fboss {
const HwAsic& getHwAsicForAsicType(const cfg::AsicType& asicType);
uint32_t getFabricPortsPerVirtualDevice(const cfg::AsicType asicType);
int getMaxNumberOfFabricPorts(const cfg::AsicType asicType);
} // namespace facebook::fboss
