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

#include <string>
#include "fboss/agent/AsyncLoggerBase.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss {

class StateDelta;

/**
 * StateDeltaLogger provides efficient logging of state deltas applied in
 * applyUpdate(). It serializes the operDelta from StateDelta objects and logs
 * them to a file asynchronously using AsyncLoggerBase.
 */
class StateDeltaLogger : public AsyncLoggerBase {
 public:
  StateDeltaLogger();
  ~StateDeltaLogger() override;

  // Delete copy constructor and assignment operator
  StateDeltaLogger(const StateDeltaLogger&) = delete;
  StateDeltaLogger& operator=(const StateDeltaLogger&) = delete;

  /**
   * Log a state delta by serializing its operDelta
   * @param delta The StateDelta to log
   * @param reason Optional reason for the state update
   */
  void logStateDelta(const StateDelta& delta, const std::string& reason);

  /**
   * Log a vector of state deltas
   * @param deltas Vector of StateDelta objects to log
   * @param reason Optional reason for the state update
   */
  void logStateDeltas(
      const std::vector<StateDelta>& deltas,
      const std::string& reason);

  void forceFlush() override;

  /**
   * Get the configured serialization protocol from gflags
   */
  static fsdb::OperProtocol getConfiguredSerializationProtocol();

 private:
  /**
   * Serialize an operDelta using the configured protocol
   * @param operDelta The OperDelta to serialize
   * @return Serialized string representation
   */
  std::string serializeOperDelta(const fsdb::OperDelta& operDelta);

  // Implement pure virtual functions from AsyncLoggerBase
  uint32_t getOffset() override;
  void setOffset(uint32_t offset) override;
  void swapCurBuf() override;

  fsdb::OperProtocol serializationProtocol_;
};

} // namespace facebook::fboss
