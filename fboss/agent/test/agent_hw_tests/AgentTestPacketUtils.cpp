/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_hw_tests/AgentTestPacketUtils.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/packet/PktFactory.h"

using namespace facebook::fboss;
namespace {
auto kDefaultPayload = std::vector<uint8_t>(256, 0xff);
} // namespace

namespace facebook::fboss::utility {} // namespace facebook::fboss::utility
