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
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"
#include "fboss/agent/test/utils/AqmTestUtils.h"
#include "fboss/agent/types.h"

/*
 * This utility is to provide utils for hw watermark tests.
 */

namespace facebook::fboss {
class HwSwitch;

namespace utility {

/*
 * Platforms might not program the buffer configurations as is or
 * read back the values of buffer usage accurately, instead is rounded.
 * Given the expected actual threshold, this API returns the rounded
 * value as programmed / read by specific platform. For BCM platforms,
 * the thresholds are always rounded up to the cell size and for TAJO,
 * the thresholds are rounded up for weatermarks and rounded DOWN for
 * ECN / WRED tresholds.
 */
int getRoundedBufferThreshold(
    HwSwitch* hwSwitch,
    int threshold,
    bool roundUp = true);

/*
 * A packet being forwarded in HW will have a buffer descriptor associated
 * with it. Also, a single packet being forwarded in HW will be split into
 * multiple ASIC specific internal buffers/cells. The effective memory usage
 * of a single packet in the ASIC would be packet size + buffer descriptor
 * size and will be in multiples of internal buffers / cells.
 */
size_t getEffectiveBytesPerPacket(HwSwitch* hwSwitch, int packetSizeInBytes);

/*
 * Get static or dynamic queue limit for platform. In case of dynamic queue
 * limit, take into account queueScalingFactor configuration and return the
 * max possible queue limit, assuming no other active queue in the system.
 */
uint32_t getQueueLimitBytes(
    HwSwitch* hwSwitch,
    std::optional<cfg::MMUScalingFactor> queueScalingFactor);

/*
 * Get shared pool size in bytes for egress, applicable for Broadcom platforms
 */
uint32_t getEgressSharedPoolLimitBytes(HwSwitch* hwSwitch);

} // namespace utility

} // namespace facebook::fboss
