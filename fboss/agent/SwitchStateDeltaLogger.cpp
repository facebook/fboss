/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/SwitchStateDeltaLogger.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <limits>

#include <fb303/ServiceData.h>
#include <folly/FBString.h>
#include <folly/hash/Checksum.h>
#include <folly/lang/Bits.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/gen-cpp2/state_delta_log_types.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

// File-scope buffer state required by AsyncLoggerBase's derived-class contract.
static auto constexpr kBufferSize = 409600;
// NOLINTBEGIN(facebook-avoid-non-const-global-variables)
static facebook::fboss::AsyncLoggerBase::BufferToWrite currentBuffer =
    facebook::fboss::AsyncLoggerBase::BufferToWrite::BUFFER0;
static uint32_t bufOffset = 0;
static std::array<char, kBufferSize> buffer0;
static std::array<char, kBufferSize> buffer1;
// NOLINTEND(facebook-avoid-non-const-global-variables)

namespace facebook::fboss {

namespace {

// On-disk record framing shared with replay tooling:
//   [magic:u32][type:u8][seq:u64][length:u32][payload][crc32:u32]
// crc32 covers (type || seq || length || payload), little-endian.
constexpr uint32_t kMagic = 0xFB0550EDu;
constexpr uint8_t kTypeBootHeader = 0x00;
constexpr uint8_t kTypeInitialState = 0x01;
constexpr uint8_t kTypeDelta = 0x02; // PRE_MANAGER_DELTA
constexpr size_t kHeaderBytes = 4 + 1 + 8 + 4;
constexpr size_t kTrailerBytes = 4;

constexpr auto kBuildRevision = "build_revision";
constexpr auto kVerboseSdkVersion = "Verbose SDK Version";

uint64_t nowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

void appendLE(folly::fbstring& out, uint8_t v) {
  out.push_back(static_cast<char>(v));
}

void appendLE(folly::fbstring& out, uint32_t v) {
  uint32_t le = folly::Endian::little<uint32_t>(v);
  out.append(reinterpret_cast<const char*>(&le), sizeof(le));
}

void appendLE(folly::fbstring& out, uint64_t v) {
  uint64_t le = folly::Endian::little<uint64_t>(v);
  out.append(reinterpret_cast<const char*>(&le), sizeof(le));
}

void appendFramedRecord(
    folly::fbstring& out,
    uint8_t type,
    uint64_t seq,
    folly::ByteRange payload) {
  const size_t framedStart = out.size();
  appendLE(out, kMagic);
  appendLE(out, type);
  appendLE(out, seq);
  CHECK_LE(payload.size(), std::numeric_limits<uint32_t>::max())
      << "payload exceeds framed record length field";
  appendLE(out, static_cast<uint32_t>(payload.size()));
  out.append(reinterpret_cast<const char*>(payload.data()), payload.size());
  // CRC covers (type || seq || length || payload) — everything after magic.
  const uint8_t* crcRegionStart = reinterpret_cast<const uint8_t*>(out.data()) +
      framedStart + sizeof(uint32_t);
  const size_t crcRegionLen = out.size() - framedStart - sizeof(uint32_t);
  const uint32_t crc = folly::crc32(crcRegionStart, crcRegionLen);
  appendLE(out, crc);
}

} // namespace

SwitchStateDeltaLogger::SwitchStateDeltaLogger()
    : AsyncLoggerBase(
          FLAGS_pre_manager_delta_log_file,
          FLAGS_pre_manager_delta_log_timeout_ms,
          AsyncLoggerBase::LoggerSrcType::STATE_DELTA,
          buffer0.data(),
          buffer1.data(),
          kBufferSize) {
  if (FLAGS_enable_pre_manager_delta_logging ||
      FLAGS_enable_post_manager_delta_logging) {
    serializationProtocol_ = getConfiguredSerializationProtocol();
    startFlushThread();
  }
}

SwitchStateDeltaLogger::~SwitchStateDeltaLogger() {
  if (FLAGS_enable_pre_manager_delta_logging ||
      FLAGS_enable_post_manager_delta_logging) {
    forceFlush();
    stopFlushThread();
  }
}

void SwitchStateDeltaLogger::logInitialState(
    const std::shared_ptr<SwitchState>& state) {
  if ((!FLAGS_enable_pre_manager_delta_logging &&
       !FLAGS_enable_post_manager_delta_logging) ||
      state == nullptr) {
    return;
  }
  auto thriftState = state->toThrift();
  auto serialized =
      apache::thrift::CompactSerializer::serialize<std::string>(thriftState);
  writeRecord(
      kTypeInitialState,
      folly::ByteRange{
          reinterpret_cast<const uint8_t*>(serialized.data()),
          serialized.size()});
}

void SwitchStateDeltaLogger::logStateDelta(const StateDelta& delta) {
  if (!FLAGS_enable_pre_manager_delta_logging) {
    return;
  }
  const uint64_t ts = nowMs();
  const int64_t oldGen =
      static_cast<int64_t>(delta.oldState()->getGeneration());
  const int64_t newGen =
      static_cast<int64_t>(delta.newState()->getGeneration());

  std::string serializedOperDelta = serializeOperDelta(delta.getOperDelta());

  StateDeltaLogRecord record;
  record.timestampMs() = ts;
  record.oldGeneration() = oldGen;
  record.newGeneration() = newGen;
  record.operDeltaBytes() =
      folly::fbstring(serializedOperDelta.data(), serializedOperDelta.size());
  record.operProtocol() = serializationProtocol_;

  auto serialized =
      apache::thrift::CompactSerializer::serialize<std::string>(record);
  writeRecord(
      kTypeDelta,
      folly::ByteRange{
          reinterpret_cast<const uint8_t*>(serialized.data()),
          serialized.size()});
}

std::string SwitchStateDeltaLogger::serializeOperDelta(
    const fsdb::OperDelta& operDelta) {
  try {
    using TC = apache::thrift::type_class::structure;
    return thrift_cow::serialize<TC>(serializationProtocol_, operDelta)
        .toStdString();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to serialize OperDelta: " << ex.what();
    throw;
  }
}

fsdb::OperProtocol
SwitchStateDeltaLogger::getConfiguredSerializationProtocol() {
  std::string protocol = FLAGS_pre_manager_delta_log_protocol;
  std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::toupper);

  if (protocol == "BINARY") {
    return fsdb::OperProtocol::BINARY;
  } else if (protocol == "SIMPLE_JSON") {
    return fsdb::OperProtocol::SIMPLE_JSON;
  } else if (protocol == "COMPACT") {
    return fsdb::OperProtocol::COMPACT;
  } else {
    XLOG(FATAL) << "Invalid pre_manager_delta_log_protocol: \""
                << FLAGS_pre_manager_delta_log_protocol
                << "\". Valid values are: BINARY, SIMPLE_JSON, COMPACT";
  }
}

void SwitchStateDeltaLogger::forceFlush() {
  if (FLAGS_enable_pre_manager_delta_logging ||
      FLAGS_enable_post_manager_delta_logging) {
    AsyncLoggerBase::forceFlush();
  }
}

void SwitchStateDeltaLogger::writeNewBootHeader() {
  BootHeader header;
  header.timestampMs() = nowMs();
  std::string commitId;
  fb303::fbData->getExportedValue(commitId, kBuildRevision);
  header.commitId() = std::move(commitId);
  header.warmBoot() = AsyncLoggerBase::getBootType();
  std::string sdkVersion;
  fb303::fbData->getExportedValue(sdkVersion, kVerboseSdkVersion);
  sdkVersion.erase(
      std::remove(sdkVersion.begin(), sdkVersion.end(), '\n'),
      sdkVersion.end());
  header.sdkVersion() = std::move(sdkVersion);

  auto serialized =
      apache::thrift::CompactSerializer::serialize<std::string>(header);
  writeRecord(
      kTypeBootHeader,
      folly::ByteRange{
          reinterpret_cast<const uint8_t*>(serialized.data()),
          serialized.size()});
}

void SwitchStateDeltaLogger::writeRecord(
    uint8_t type,
    folly::ByteRange payload) {
  folly::fbstring buffer;
  {
    std::lock_guard<std::mutex> guard(writeLock_);
    const uint64_t seq = seqNum_.fetch_add(1, std::memory_order_relaxed);
    appendFramedRecord(buffer, type, seq, payload);
    appendLog(buffer.data(), buffer.size());
  }
}

uint32_t SwitchStateDeltaLogger::getOffset() {
  return bufOffset;
}

void SwitchStateDeltaLogger::setOffset(uint32_t offset) {
  bufOffset = offset;
}

void SwitchStateDeltaLogger::swapCurBuf() {
  currentBuffer = currentBuffer == AsyncLoggerBase::BUFFER0
      ? AsyncLoggerBase::BUFFER1
      : AsyncLoggerBase::BUFFER0;
}

} // namespace facebook::fboss
