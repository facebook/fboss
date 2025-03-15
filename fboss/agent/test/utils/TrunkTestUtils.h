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

#include <stdint.h>

namespace facebook::fboss {

class AgentEnsemble;
struct AggregatePortID;

namespace utility {

bool verifyAggregatePortMemberCount(
    AgentEnsemble& ensemble,
    AggregatePortID aggregatePortID,
    uint8_t currentCount);

int getAggregatePortCount(AgentEnsemble& ensemble);
bool verifyAggregatePort(AgentEnsemble& ensemble, AggregatePortID aggId);
bool verifyPktFromAggregatePort(AgentEnsemble& ensemble, AggregatePortID aggId);

} // namespace utility
} // namespace facebook::fboss
