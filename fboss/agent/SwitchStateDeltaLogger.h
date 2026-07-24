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

#include <folly/Range.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include "fboss/agent/AsyncLoggerBase.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss {

class StateDelta;
class SwitchState;

class SwitchStateDeltaLogger : public AsyncLoggerBase {
 public:
  SwitchStateDeltaLogger();
  ~SwitchStateDeltaLogger() override;

  SwitchStateDeltaLogger(const SwitchStateDeltaLogger&) = delete;
  SwitchStateDeltaLogger& operator=(const SwitchStateDeltaLogger&) = delete;

  void logInitialState(const std::shared_ptr<SwitchState>& state);
  void logStateDelta(const StateDelta& delta);
  void forceFlush() override;

  static fsdb::OperProtocol getConfiguredSerializationProtocol();

 protected:
  void writeNewBootHeader() override;

 private:
  std::string serializeOperDelta(const fsdb::OperDelta& operDelta);
  void writeRecord(uint8_t type, folly::ByteRange payload);

  uint32_t getOffset() override;
  void setOffset(uint32_t offset) override;
  void swapCurBuf() override;

  fsdb::OperProtocol serializationProtocol_;
  std::atomic<uint64_t> seqNum_{0};
  // Serializes writers so each framed record lands contiguously in the buffer.
  std::mutex writeLock_;
};

} // namespace facebook::fboss
