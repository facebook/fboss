// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/module/I2cLogBuffer.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

namespace {
constexpr size_t kMicrosecondsPerSecond = 1000000;
const std::string kEmptyOptional = "...";
} // namespace

namespace facebook::fboss {

I2cLogBuffer::I2cLogBuffer(
    cfg::TransceiverI2cLogging config,
    std::string logFile)
    : buffer_(config.bufferSlots().value()),
      size_(config.bufferSlots().value()),
      config_(config),
      logFile_(logFile) {
  if (size_ == 0) {
    throw std::invalid_argument("I2cLogBuffer size must be > 0");
  }
  if (logFile_.empty()) {
    throw std::invalid_argument("I2cLogBuffer logFile must be non-empty");
  }
}

void I2cLogBuffer::log(
    const TransceiverAccessParameter& param,
    const uint8_t* data,
    Operation op,
    bool success) {
  if (data == nullptr) {
    throw std::invalid_argument("I2cLogBuffer data must be non-null");
  }
  std::lock_guard<std::mutex> g(mutex_);
  if ((op == Operation::Read && config_.readLog().value()) ||
      (op == Operation::Write && config_.writeLog().value())) {
    buffer_[head_].steadyTime = std::chrono::steady_clock::now();
    buffer_[head_].systemTime = std::chrono::system_clock::now();
    buffer_[head_].param = param;
    const size_t len = std::min(param.len, kMaxI2clogDataSize);
    auto& bufferData = buffer_[head_].data;
    std::copy(data, data + len, bufferData.begin());
    if (len < kMaxI2clogDataSize) {
      std::fill(bufferData.begin() + len, bufferData.end(), 0);
    }
    buffer_[head_].op = op;
    buffer_[head_].success = success;

    // Let tail track the oldest entry.
    if ((head_ == tail_) && (totalEntries_ != 0)) {
      tail_ = (tail_ + 1) % size_;
    }
    // advance head_
    head_ = (head_ + 1) % size_;
    totalEntries_++;
  }

  if (!success && config_.disableOnFail().value()) {
    config_.readLog() = false;
    config_.writeLog() = false;
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

size_t I2cLogBuffer::getHeader(
    std::stringstream& ss,
    size_t entries,
    size_t numContents) {
  // Format of the log. All printed header lines must be terminated with \n to
  // return the right number of lines in header.
  ss << "I2cLogBuffer: Total Entries: " << entries << " Logged: " << numContents
     << "\n";
  ss << "Between the Operation <Param> and [Data], an 'F' indicates a failure in the transaction.\n";
  ss << "If the read transaction failed [Data] may not be accurate.\n";
  ss << "Header format: \n";
  ss << "Month D HH:MM:SS.mmmuuu <i2c_address  offset  len  page  bank  op> F [data]  steadyclock_ns"
     << "\n";

  auto str = ss.str();
  return std::count(str.begin(), str.end(), '\n');
}

void I2cLogBuffer::getEntryTime(
    std::stringstream& ss,
    const TimePointSystem& time_point) {
  std::time_t t = std::chrono::system_clock::to_time_t(time_point);
  std::tm tm{};
  // get the Month day HH:MM:SS
  localtime_r(&t, &tm);
  // get the microseconds
  auto us =
      duration_cast<std::chrono::microseconds>(time_point.time_since_epoch()) %
      kMicrosecondsPerSecond;

  ss << std::put_time(&tm, "%B %d %H:%M:%S");
  ss << "." << std::setfill('0') << std::setw(6) << us.count();
}

template <typename T>
void I2cLogBuffer::getOptional(std::stringstream& ss, T value) {
  if (value.has_value()) {
    ss << std::setfill(' ') << std::setw(3) << (int)value.value();
  } else {
    ss << kEmptyOptional;
  }
  ss << " ";
}

std::pair<size_t, size_t> I2cLogBuffer::dumpToFile() {
  std::vector<I2cLogEntry> entriesOut(size_);
  auto entries = totalEntries_;
  const size_t numContents = dump(entriesOut);
  std::stringstream ss;

  auto hdrSize = getHeader(ss, entries, numContents);

  TimePointSteady prev;

  for (size_t i = 0; i < numContents; i++) {
    getEntryTime(ss, entriesOut[i].systemTime);
    ss << " <";
    auto& param = entriesOut[i].param;
    getOptional(ss, param.i2cAddress);
    ss << std::setfill(' ') << std::setw(3) << param.offset << " ";
    ss << std::setfill(' ') << std::setw(3) << param.len << " ";
    getOptional(ss, param.page);
    getOptional(ss, param.bank);
    ss << (entriesOut[i].op == Operation::Read ? "R" : "W");
    ss << "> ";
    if (entriesOut[i].success) {
      ss << " ";
    } else {
      ss << "F";
    }
    ss << " [";

    for (auto& data : entriesOut[i].data) {
      ss << std::hex << std::setfill('0') << std::setw(2) << (uint16_t)data;
    }
    ss << "] ";

    ss << std::dec;
    if (i == 0) {
      ss << 0;
    } else {
      ss << std::chrono::duration_cast<std::chrono::nanoseconds>(
                entriesOut[i].steadyTime - prev)
                .count();
    }
    prev = entriesOut[i].steadyTime;
    ss << std::endl;
  }
  folly::writeFile(ss.str(), logFile_.c_str());
  return std::make_pair(hdrSize, numContents);
}

TransceiverAccessParameter I2cLogBuffer::getParam(std::stringstream& ss) {
  TransceiverAccessParameter param(0, 0, 0);
  std::string token;
  ss >> token;
  if (token != kEmptyOptional) {
    param.i2cAddress = folly::to<uint8_t>(token);
  }
  ss >> param.offset;
  ss >> param.len;
  ss >> token;
  if (token != kEmptyOptional) {
    param.page = folly::to<uint8_t>(token);
  }
  ss >> token;
  if (token != kEmptyOptional) {
    param.bank = folly::to<uint8_t>(token);
  }
  return param;
}

I2cLogBuffer::Operation I2cLogBuffer::getOp(std::stringstream& ss) {
  char c;
  ss >> c;
  switch (c) {
    case 'R':
      return Operation::Read;
      break;
    case 'W':
      return Operation::Write;
      break;
    default:
      throw std::invalid_argument(fmt::format("Invalid Operation :{}", c));
      break;
  }
}

std::array<uint8_t, kMaxI2clogDataSize> I2cLogBuffer::getData(std::string str) {
  if (str.size() != kMaxI2clogDataSize * 2) {
    throw(std::invalid_argument("Invalid data length:" + str));
  }
  std::array<uint8_t, kMaxI2clogDataSize> arr;

  for (size_t i = 0; i < str.length(); i += 2) {
    std::string byteString = str.substr(i, 2);
    uint8_t byte = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
    arr[i / 2] = byte;
  }
  return arr;
}

uint64_t I2cLogBuffer::getDelay(const std::string& str) {
  size_t pos = str.rfind(' ');
  if (pos != std::string::npos) {
    return stoull(str.substr(pos + 1));
  } else {
    throw std::invalid_argument("Invalid delay:" + str);
  }
}

bool I2cLogBuffer::getSuccess(const std::string& str) {
  if (str == " F ") {
    return false;
  }
  return true;
}

std::string
I2cLogBuffer::getField(const std::string& line, char left, char right) {
  auto leftIndex = line.find(left);
  auto rightIndex = line.find(right);
  if (leftIndex == std::string::npos || rightIndex == std::string::npos ||
      leftIndex >= rightIndex) {
    throw std::invalid_argument(
        fmt::format("Invalid log file format: {} {} {}", line, left, right));
  }
  return line.substr(leftIndex + 1, rightIndex - leftIndex - 1);
}

std::vector<I2cLogBuffer::I2cReplayEntry> I2cLogBuffer::loadFromLog(
    std::string logFile) {
  std::vector<I2cReplayEntry> entries;
  std::string line;
  std::ifstream file(logFile);
  if (!file.is_open()) {
    XLOG(ERR) << "Failed to open file " << logFile;
    return entries;
  }

  std::stringstream ss;
  auto hdrSize = getHeader(ss, 0, 0);
  for (auto i = 0; i < hdrSize; i++) {
    if (!std::getline(file, line)) {
      XLOG(ERR) << "Failed to read log header from file " << logFile;
      return entries;
    }
  }

  while (std::getline(file, line)) {
    ss = std::stringstream(getField(line, '<', '>'));
    auto param = getParam(ss);
    auto op = getOp(ss);
    auto str = getField(line, '[', ']');
    auto data = getData(str);
    auto delay = getDelay(line);
    str = getField(line, '>', '[');
    auto success = getSuccess(str);
    entries.emplace_back(param, op, data, delay, success);
  }
  return entries;
}

} // namespace facebook::fboss
