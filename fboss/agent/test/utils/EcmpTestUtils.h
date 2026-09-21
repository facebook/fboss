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

#include "fboss/agent/test/AgentEnsemble.h"

namespace facebook::fboss::utility {

std::map<int32_t, int32_t> getEcmpWeightsInHw(
    AgentEnsemble* ensemble,
    facebook::fboss::utility::CIDRNetwork cidr);

int getEcmpSizeInHw(
    AgentEnsemble* ensemble,
    facebook::fboss::utility::CIDRNetwork cidr);

} // namespace facebook::fboss::utility
