// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/IOStatsRecorder.h"
#include "fboss/lib/if/gen-cpp2/io_stats_types.h"

namespace facebook {
namespace fboss {

void IOStatsRecorder::updateReadDownTime() {
  std::lock_guard<std::mutex> g(statsMutex_);
  stats_.readDownTime() = downTimeLocked(lastSuccessfulRead_);
}

void IOStatsRecorder::updateWriteDownTime() {
  std::lock_guard<std::mutex> g(statsMutex_);
  stats_.writeDownTime() = downTimeLocked(lastSuccessfulWrite_);
}

void IOStatsRecorder::recordReadSuccess() {
  std::lock_guard<std::mutex> g(statsMutex_);
  lastSuccessfulRead_ = std::chrono::steady_clock::now();
}

void IOStatsRecorder::recordWriteSuccess() {
  std::lock_guard<std::mutex> g(statsMutex_);
  lastSuccessfulWrite_ = std::chrono::steady_clock::now();
}

void IOStatsRecorder::recordReadAttempted() {
  std::lock_guard<std::mutex> g(statsMutex_);
  stats_.numReadAttempted() = stats_.numReadAttempted().value() + 1;
}

void IOStatsRecorder::recordWriteAttempted() {
  std::lock_guard<std::mutex> g(statsMutex_);
  stats_.numWriteAttempted() = stats_.numWriteAttempted().value() + 1;
}

void IOStatsRecorder::recordReadFailed() {
  std::lock_guard<std::mutex> g(statsMutex_);
  stats_.numReadFailed() = stats_.numReadFailed().value() + 1;
}

void IOStatsRecorder::recordWriteFailed() {
  std::lock_guard<std::mutex> g(statsMutex_);
  stats_.numWriteFailed() = stats_.numWriteFailed().value() + 1;
}

IOStats IOStatsRecorder::getStats() {
  std::lock_guard<std::mutex> g(statsMutex_);
  return stats_;
}

} // namespace fboss
} // namespace facebook
