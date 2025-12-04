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

// Static buffers for async logging
static auto constexpr kBufferSize = 409600;
static facebook::fboss::AsyncLoggerBase::BufferToWrite currentBuffer =
    facebook::fboss::AsyncLoggerBase::BufferToWrite::BUFFER0;
static uint32_t bufOffset = 0;
static std::array<char, kBufferSize> buffer0;
static std::array<char, kBufferSize> buffer1;

namespace facebook::fboss {

StateDeltaLogger::StateDeltaLogger()
    : AsyncLoggerBase(
          FLAGS_state_delta_log_file,
          FLAGS_state_delta_log_timeout_ms,
          AsyncLoggerBase::LoggerSrcType::STATE_DELTA,
          buffer0.data(),
          buffer1.data(),
          kBufferSize) {
  if (FLAGS_enable_state_delta_logging) {
    serializationProtocol_ = getConfiguredSerializationProtocol();
    startFlushThread();
  }
}

StateDeltaLogger::~StateDeltaLogger() {
  if (FLAGS_enable_state_delta_logging) {
    forceFlush();
    stopFlushThread();
  }
}

void StateDeltaLogger::logStateDelta(
    const StateDelta& delta,
    const std::string& reason) {
  if (!FLAGS_enable_state_delta_logging) {
    return;
  }
  const auto& operDelta = delta.getOperDelta();
  auto serializedDelta = serializeOperDelta(operDelta);

  std::ostringstream logEntry;
  logEntry << "{" << "\"timestamp\":"
           << std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count()
           << "," << "\"reason\":\"" << reason << "\","
           << "\"old_generation\":" << delta.oldState()->getGeneration() << ","
           << "\"new_generation\":" << delta.newState()->getGeneration() << ","
           << "\"oper_delta_size\":" << serializedDelta.size() << ","
           << "\"oper_delta\":\"" << serializedDelta << "\"" << "}\n";

  auto logEntryStr = logEntry.str();
  appendLog(logEntryStr.c_str(), logEntryStr.size());
}

std::string StateDeltaLogger::serializeOperDelta(
    const fsdb::OperDelta& operDelta) {
  try {
    // Use the thrift_cow serializer to serialize the operDelta
    using TC = apache::thrift::type_class::structure;
    return thrift_cow::serialize<TC>(serializationProtocol_, operDelta)
        .toStdString();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to serialize OperDelta: " << ex.what();
    throw;
  }
}

void StateDeltaLogger::logStateDeltas(
    const std::vector<StateDelta>& deltas,
    const std::string& reason) {
  for (const auto& delta : deltas) {
    logStateDelta(delta, reason);
  }
}

fsdb::OperProtocol StateDeltaLogger::getConfiguredSerializationProtocol() {
  std::string protocol = FLAGS_state_delta_log_protocol;

  // Convert to uppercase for case-insensitive comparison
  std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::toupper);

  if (protocol == "BINARY") {
    return fsdb::OperProtocol::BINARY;
  } else if (protocol == "SIMPLE_JSON") {
    return fsdb::OperProtocol::SIMPLE_JSON;
  } else if (protocol == "COMPACT") {
    return fsdb::OperProtocol::COMPACT;
  } else {
    XLOG(FATAL) << "Invalid state_delta_log_protocol: \""
                << FLAGS_state_delta_log_protocol
                << "\". Valid values are: BINARY, SIMPLE_JSON, COMPACT";
  }
}

void StateDeltaLogger::forceFlush() {
  if (FLAGS_enable_state_delta_logging) {
    AsyncLoggerBase::forceFlush();
  }
}

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
