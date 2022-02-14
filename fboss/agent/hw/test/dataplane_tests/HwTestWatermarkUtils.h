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
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/types.h"

/*
 * This utility is to provide utils for hw watermark tests.
 */

namespace facebook::fboss {
class HwSwitch;

namespace utility {

/*
 * A packet being forwarded in HW will have a buffer descriptor associated
 * with it. Also, a single packet being forwarded in HW will be split into
 * multiple ASIC specific internal buffers/cells. The effective memory usage
 * of a single packet in the ASIC would be packet size + buffer descriptor
 * size and will be in multiples of internal buffers / cells.
 */
size_t getEffectiveBytesPerPacket(HwSwitch* hwSwitch, int packetSizeInBytes);

} // namespace utility
} // namespace facebook::fboss
