// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/module/I2cLogBuffer.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

#include "fboss/qsfp_service/module/cmis/gen-cpp2/cmis_types.h"
#include "fboss/qsfp_service/module/sff/gen-cpp2/sff8472_types.h"
#include "fboss/qsfp_service/module/sff/gen-cpp2/sff_types.h"

namespace {
constexpr size_t kMicrosecondsPerSecond = 1000000;
const std::string kEmptyOptional = "...";
} // namespace

namespace facebook::fboss {

I2cLogBuffer::I2cLogBuffer(
    cfg::TransceiverI2cLogging config,
    std::string logFile)
    : size_(
          config.bufferSlots().value() > kMaxNumberBufferSlots
              ? kMaxNumberBufferSlots
              : config.bufferSlots().value()),
      writeLog_(config.writeLog().value()),
      readLog_(config.readLog().value()),
      disableOnFail_(config.disableOnFail().value()),
      buffer_(size_),
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
    const int field,
    const uint8_t* data,
    Operation op,
    bool success) {
  if (data == nullptr) {
    throw std::invalid_argument("I2cLogBuffer data must be non-null");
  }
  std::lock_guard<std::mutex> g(mutex_);
  if ((op == Operation::Read && readLog_) ||
      (op == Operation::Write && writeLog_) || (op == Operation::Reset) ||
      ((op == Operation::Presence))) {
    auto& bufferHead = buffer_[head_];
    bufferHead.steadyTime = std::chrono::steady_clock::now();
    bufferHead.systemTime = std::chrono::system_clock::now();
    bufferHead.param = param;
    bufferHead.field = field;
    const size_t len = std::min(param.len, kMaxI2clogDataSize);
    auto& bufferData = bufferHead.data;
    std::copy(data, data + len, bufferData.begin());
    if (len < kMaxI2clogDataSize) {
      std::fill(bufferData.begin() + len, bufferData.end(), 0);
    }
    bufferHead.op = op;
    bufferHead.success = success;

    // Let tail track the oldest entry.
    if ((head_ == tail_) && (totalEntries_ != 0)) {
      tail_ = (tail_ + 1) % size_;
    }
    // advance head_
    head_ = (head_ + 1) % size_;
    totalEntries_++;
  }

  if (!success && disableOnFail_) {
    readLog_ = false;
    writeLog_ = false;
  }
}

I2cLogBuffer::I2cLogHeader I2cLogBuffer::dump(
    std::vector<I2cLogEntry>& entriesOut) {
  std::lock_guard<std::mutex> g(mutex_);

  auto start = std::chrono::high_resolution_clock::now();

  if (entriesOut.size() != size_) {
    entriesOut.resize(size_);
  }

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

  auto end = std::chrono::high_resolution_clock::now();
  auto copyTime =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  I2cLogHeader retval = {
      .mgmtIf = mgmtIf_,
      .totalEntries = totalEntries_,
      .bufferEntries = entries,
      .portNames = portNames_,
      .fwStatus = fwStatus_,
      .vendor = vendor_,
      .duration = copyTime};
  totalEntries_ = head_ = tail_ = 0;
  return retval;
}

size_t I2cLogBuffer::getHeader(
    std::stringstream& ss,
    const I2cLogHeader& info) {
  // Format of the log. All printed header lines must be terminated with \n to
  // return the right number of lines in header.
  if (auto fw = info.fwStatus) {
    if (fw.value().version().has_value()) {
      ss << "FW Version: "
         << *apache::thrift::get_pointer(fw.value().version());
    }
    if (fw.value().dspFwVer().has_value()) {
      ss << " DSP FW Version: "
         << *apache::thrift::get_pointer(fw.value().dspFwVer());
    }
    if (fw.value().buildRev().has_value()) {
      ss << " FW Build Revision: "
         << *apache::thrift::get_pointer(fw.value().buildRev());
    }
  }
  if (auto vendor = info.vendor) {
    ss << " Vendor: " << vendor.value().name().value()
       << " Part Number: " << vendor.value().partNumber().value()
       << " Serial Number: " << vendor.value().serialNumber().value();
  }
  ss << " Management Interface: "
     << apache::thrift::util::enumNameSafe(info.mgmtIf);
  ss << "\n";
  ss << "Port Names: " << folly::join(" ", info.portNames) << "\n";
  ss << "I2cLogBuffer: Total Entries: " << info.totalEntries
     << " Logged: " << info.bufferEntries
     << " Copy Lock Time: " << info.duration.count() << " us\n";
  ss << "Between the Operation <Param> and [Data], an 'F' indicates a failure in the transaction.\n";
  ss << "If the read transaction failed [Data] may not be accurate.\n";
  ss << "mmmuuu: milliseconds microseconds, steadclock_ns: time in ns between log entries \n";
  ss << "Header format: \n";
  ss << "Month D HH:MM:SS.mmmuuu FIELD_NAME <i2c_address page bank offset len op> F [data]  steadyclock_ns"
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

void I2cLogBuffer::getFieldName(std::stringstream& ss, const int field) {
  std::string fieldName;
  switch (mgmtIf_) {
    case TransceiverManagementInterface::CMIS:
      fieldName =
          apache::thrift::util::enumNameSafe(static_cast<CmisField>(field));
      break;
    case TransceiverManagementInterface::SFF:
      fieldName =
          apache::thrift::util::enumNameSafe(static_cast<SffField>(field));
      break;
    case TransceiverManagementInterface::SFF8472:
      fieldName =
          apache::thrift::util::enumNameSafe(static_cast<Sff8472Field>(field));
      break;
    default:
      break;
  }

  if (!fieldName.empty()) {
    ss << std::setfill(' ') << std::setw(kI2cFieldNameLength)
       << fieldName.substr(0, kI2cFieldNameLength);
  }
}

template <typename T>
void I2cLogBuffer::getOptional(std::stringstream& ss, T value) {
  if (value.has_value()) {
    ss << std::setfill(' ') << std::setw(3) << static_cast<int>(value.value());
  } else {
    ss << kEmptyOptional;
  }
  ss << " ";
}

std::pair<size_t, size_t> I2cLogBuffer::dumpToFile() {
  // To avoid high latency for lock (during memory allocation), the thrift API
  // call will run this function and the entriesOut will be initialized to the
  // right size before the call to dump();
  const size_t size = getSize();
  std::vector<I2cLogEntry> entriesOut(size);
  const auto headerInfo = dump(entriesOut);
  std::stringstream ss;

  auto logCount = headerInfo.bufferEntries;
  auto hdrSize = getHeader(ss, headerInfo);

  TimePointSteady prev;

  for (size_t i = 0; i < logCount; i++) {
    auto& entry = entriesOut[i];
    getEntryTime(ss, entry.systemTime);
    ss << " ";
    getFieldName(ss, entry.field);
    ss << " <";
    auto& param = entry.param;
    getOptional(ss, param.i2cAddress);
    getOptional(ss, param.page);
    getOptional(ss, param.bank);
    ss << std::setfill(' ') << std::setw(3) << param.offset << " ";
    ss << std::setfill(' ') << std::setw(3) << param.len << " ";
    std::string opChar;
    switch (entry.op) {
      case Operation::Read:
        opChar = "R";
        break;
      case Operation::Write:
        opChar = "W";
        break;
      case Operation::Reset:
        opChar = "T";
        break;
      case Operation::Presence:
        opChar = "P";
        break;
    }
    ss << opChar;
    ss << "> ";
    if (entry.success) {
      ss << " ";
    } else {
      ss << "F";
    }
    ss << " [";

    for (auto& data : entry.data) {
      ss << std::hex << std::setfill('0') << std::setw(2) << (uint16_t)data;
    }
    ss << "] ";

    ss << std::dec;
    if (i == 0) {
      ss << 0;
    } else {
      ss << std::chrono::duration_cast<std::chrono::nanoseconds>(
                entry.steadyTime - prev)
                .count();
    }
    prev = entry.steadyTime;
    ss << std::endl;
  }
  folly::writeFile(ss.str(), logFile_.c_str());
  return std::make_pair(hdrSize, logCount);
}

TransceiverAccessParameter I2cLogBuffer::getParam(const std::string& str) {
  TransceiverAccessParameter param(0, 0, 0);
  std::vector<std::string> fields;
  folly::split(' ', str, fields, true);
  if (fields.size() < kNumParamFields) {
    throw std::out_of_range("Invalie param fields:" + str);
  }
  if (fields.at(0) != kEmptyOptional) {
    param.i2cAddress = folly::to<uint8_t>(fields.at(0));
  };
  if (fields.at(1) != kEmptyOptional) {
    param.page = folly::to<int>(fields.at(1));
  }
  if (fields.at(2) != kEmptyOptional) {
    param.bank = folly::to<int>(fields.at(2));
  }
  param.offset = folly::to<int>(fields.at(3));
  param.len = folly::to<int>(fields.at(4));
  return param;
}

I2cLogBuffer::Operation I2cLogBuffer::getOp(const char op) {
  switch (op) {
    case 'R':
      return Operation::Read;
    case 'W':
      return Operation::Write;
    case 'T':
      return Operation::Reset;
    case 'P':
      return Operation::Presence;
  }
  throw std::invalid_argument(fmt::format("Invalid Operation :{}", op));
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
  I2cLogHeader header = {};
  auto hdrSize = getHeader(ss, header);
  for (auto i = 0; i < hdrSize; i++) {
    if (!std::getline(file, line)) {
      XLOG(ERR) << "Failed to read log header from file " << logFile;
      return entries;
    }
  }

  while (std::getline(file, line)) {
    auto str = getField(line, '<', '>');
    auto param = getParam(str);
    auto op = getOp(str.back());
    str = getField(line, '[', ']');
    auto data = getData(str);
    auto delay = getDelay(line);
    str = getField(line, '>', '[');
    auto success = getSuccess(str);
    entries.emplace_back(param, op, data, delay, success);
  }
  return entries;
}

void I2cLogBuffer::setTcvrInfoInLog(
    const TransceiverManagementInterface& mgmtIf,
    const std::set<std::string>& portNames,
    const std::optional<FirmwareStatus>& status,
    const std::optional<Vendor>& vendor) {
  std::lock_guard<std::mutex> g(mutex_);
  mgmtIf_ = mgmtIf;
  portNames_ = portNames;
  fwStatus_ = status;
  vendor_ = vendor;
}

} // namespace facebook::fboss
