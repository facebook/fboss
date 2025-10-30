/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/StateDeltaLogger.h"

#include <chrono>
#include <sstream>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/logging/xlog.h>

DEFINE_bool(
    enable_state_delta_logging,
    false,
    "Enable logging of state deltas applied in applyUpdate()");

DEFINE_string(
    state_delta_log_file,
    "/tmp/state_delta.log",
    "Path to the state delta log file.");

DEFINE_string(
    state_delta_log_protocol,
    "COMPACT",
    "Serialization protocol for state delta logging (BINARY, SIMPLE_JSON, COMPACT)");

// Static buffers for async logging
static auto constexpr kBufferSize = 409600;
static facebook::fboss::AsyncLoggerBase::BufferToWrite currentBuffer =
    facebook::fboss::AsyncLoggerBase::BufferToWrite::BUFFER0;
static uint32_t bufOffset = 0;
static std::array<char, kBufferSize> buffer0;
static std::array<char, kBufferSize> buffer1;

namespace facebook::fboss {

uint32_t StateDeltaLogger::getOffset() {
  return bufOffset;
}

void StateDeltaLogger::setOffset(uint32_t offset) {
  bufOffset = offset;
}

void StateDeltaLogger::swapCurBuf() {
  currentBuffer = currentBuffer == AsyncLoggerBase::BUFFER0
      ? AsyncLoggerBase::BUFFER1
      : AsyncLoggerBase::BUFFER0;
}
} // namespace facebook::fboss
