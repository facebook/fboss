// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/module/I2cLogBuffer.h"

#include <folly/logging/xlog.h>
#include <algorithm>

namespace facebook::fboss {

I2cLogBuffer::I2cLogBuffer(cfg::TransceiverI2cLogging config)
    : buffer_(config.bufferSlots().value()),
      size_(config.bufferSlots().value()),
      config_(config) {
  if (size_ == 0) {
    throw std::invalid_argument("I2cLogBuffer size must be > 0");
  }
}

void I2cLogBuffer::log(
    const TransceiverAccessParameter& param,
    const uint8_t* data,
    Operation op) {
  if (data == nullptr) {
    throw std::invalid_argument("I2cLogBuffer data must be non-null");
  }
  std::lock_guard<std::mutex> g(mutex_);
  if ((op == Operation::Read && config_.readLog().value()) ||
      (op == Operation::Write && config_.writeLog().value())) {
    buffer_[head_].steadyTime = std::chrono::steady_clock::now();
    buffer_[head_].systemTime = std::chrono::system_clock::now();
    buffer_[head_].param = param;
    std::copy(
        data,
        data + std::min(kMaxI2clogDataSize, param.len),
        buffer_[head_].data.begin());
    buffer_[head_].op = op;

    // Let tail track the oldest entry.
    if ((head_ == tail_) && (totalEntries_ != 0)) {
      tail_ = (tail_ + 1) % size_;
    }
    // advance head_
    head_ = (head_ + 1) % size_;
    totalEntries_++;
  }
}

size_t I2cLogBuffer::dump(std::vector<I2cLogEntry>& entriesOut) {
  // resize the vector before locking the mutex. This is to avoid
  // memory allocation while locking the mutex.
  entriesOut.resize(size_);

  std::lock_guard<std::mutex> g(mutex_);
  size_t entries = 0;
  // Copy entries from tail to head.
  // If head < tail:
  //    Copy around the buffer: [tail -> size], [0 -> head).
  // If head > tail:
  //    Copy contents once: [tail -> head).
  // If head == tail with entries:
  //    Copy around the buffer: [head -> size], [0 -> head).
  //    (if head == 0) we just copy entire buffer [Beg -> end].
  if (head_ < tail_) {
    std::copy(buffer_.begin() + tail_, buffer_.end(), entriesOut.begin());
    entries = size_ - tail_;
    std::copy(
        buffer_.begin(), buffer_.begin() + head_, entriesOut.begin() + entries);
    entries += head_;
  } else if (head_ > tail_) {
    std::copy(
        buffer_.begin() + tail_, buffer_.begin() + head_, entriesOut.begin());
    entries = head_ - tail_;
  } else if ((head_ == tail_ && totalEntries_ != 0)) {
    entries = size_ - head_;
    std::copy(buffer_.begin() + head_, buffer_.end(), entriesOut.begin());
    if (head_ != 0) {
      // Go around the buffer.
      std::copy(
          buffer_.begin(),
          buffer_.begin() + head_,
          entriesOut.begin() + entries);
      entries += head_;
    }
  }

  totalEntries_ = head_ = tail_ = 0;
  return entries;
}

void I2cLogBuffer::transactionError() {
  std::lock_guard<std::mutex> g(mutex_);
  if (config_.disableOnFail().value()) {
    config_.readLog() = false;
    config_.writeLog() = false;
  }
}
} // namespace facebook::fboss
