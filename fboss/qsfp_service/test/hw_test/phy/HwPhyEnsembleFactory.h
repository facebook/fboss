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

#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"

#include <memory>

namespace facebook::fboss {

/*
 * API to create appropriate HW PHY ensemble based on the platform
 * This provides a runtime hook to dynamically plug in various
 * PhyManager, platform implementations and then interact with
 * them through the generic HwPhyEnsemble (and contained
 * members - PhyManager) interfaces.
 */
std::unique_ptr<HwPhyEnsemble> createHwEnsemble();
} // namespace facebook::fboss
