/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ResourceAccountant.h"

DEFINE_int32(
    ecmp_resource_percentage,
    75,
    "Percentage of ECMP resources (out of 100) allowed to use before ResourceAccountant rejects the update.");

namespace facebook::fboss {

ResourceAccountant::ResourceAccountant(const HwAsicTable* asicTable)
    : asicTable_(asicTable) {}

} // namespace facebook::fboss
