// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <array>
#include <chrono>
#include <mutex>
#include <vector>

#include "fboss/lib/usb/TransceiverAccessParameter.h"

#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"

namespace facebook::fboss {

/*
 * Class to log I2c transactions to a buffer. An instance of this class can
 * be created per transceiver to track latest <size_> transactions written
 * to the transceiver.
 * This class is designed as a circular buffer with the following properties:
 *     * Can hold up to <size_> entries.
 *     * The buffer is full when the head_ == tail_ and there are elements
 *       inserted.
 *     * Tail will track the oldest element still in buffer.
 *     * When dumping the contents of the buffer, the buffer is cleared.
 *     * The buffer is thread safe (logging/dumping)
 *     * The size of valid data in each buffer slot is:
 *       min(kMaxI2clogDataSize, param.len)
 */

constexpr int kMaxI2clogDataSize = 128;
constexpr size_t kI2cFieldNameLength = 16;
// Maximum number of buffer slots. Note that depending on the size of the log
// element, to maintain a reasonable memory usage, we have to limit max size
// the buffer.
constexpr int kMaxNumberBufferSlots = 40960;

// Number of address fields in TransceiverAccessParameter
constexpr int kNumParamFields = 5;

class I2cLogBuffer {
  using TimePointSteady = std::chrono::steady_clock::time_point;
  using TimePointSystem = std::chrono::system_clock::time_point;

 public:
  enum class Operation {
    Read = 0,
    Write = 1,
    Reset = 2,
    Presence = 3,
  };

  // Log Entry.
  // Steady time required for replaying the sequence of transactions.
  // System time required to correlate to other qsfp_service logs
  struct I2cLogEntry {
    TimePointSteady steadyTime;
    TimePointSystem systemTime;
    TransceiverAccessParameter param;
    int field;
    std::array<uint8_t, kMaxI2clogDataSize> data;
    Operation op;
    bool success{true};
    // Using default constructor for TransceiverAccessParameter when
    // initializing the buffer.
    I2cLogEntry() : param(TransceiverAccessParameter(0, 0, 0)) {}
  };

  struct I2cLogHeader {
    TransceiverManagementInterface mgmtIf;
    size_t totalEntries;
    size_t bufferEntries;
    std::set<std::string> portNames;
    std::optional<FirmwareStatus> fwStatus;
    std::optional<Vendor> vendor;
    std::chrono::microseconds duration;
  };

  // NOTE: The maximum number of entries is defined in config (qsfp_config.cinc)
  // and also limited by kMaxNumberBufferSlots.
  static_assert(
      sizeof(I2cLogEntry) < 200,
      "I2cLogEntry must be < 200B to not exceed system memory limits.");

  struct I2cReplayEntry {
    TransceiverAccessParameter param;
    Operation op;
    std::array<uint8_t, kMaxI2clogDataSize> data;
    uint64_t delay;
    bool success{true};
    I2cReplayEntry(
        TransceiverAccessParameter param,
        Operation op,
        std::array<uint8_t, kMaxI2clogDataSize> data,
        uint64_t delay,
        bool success)
        : param(param),
          op(op),
          data(std::move(data)),
          delay(delay),
          success(success) {}
  };

  explicit I2cLogBuffer(cfg::TransceiverI2cLogging config, std::string logFile);

  // Insert a log entry into the buffer.
  void log(
      const TransceiverAccessParameter& param,
      const int field,
      const uint8_t* data,
      Operation op,
      bool success = true);

  // Dump the buffer contents into a vector of I2cLogEntry.
  // The vector (enriesOut) will be resized to the size of buffers,
  // and return value represents the number of valid entries dumped.
  // The log entries in the vector are from oldest to newest time.
  // Once the buffer is dumped, the contents of the circular buffer
  // will be cleared.
  // It is recommended that entriesOut is created with capacity <size_>
  // on the stack before calling this function (reduce allocation latency).
  // Returns a pair: total enties logged and number of entries in circular
  // buffer. Total entries logged could be much higher than capacity of circular
  // buffer.
  I2cLogHeader dump(std::vector<I2cLogEntry>& entriesOut);

  // Get the number of entries logged to the buffer. The size of the
  // buffer can be smaller than total entries logged.
  size_t getTotalEntries() const {
    return totalEntries_;
  }

  // Get the capacity
  size_t getI2cLogBufferCapacity() const {
    return getSize();
  }

  // Dumps the buffer contents into logFile_.
  // Format: Each log entry will be dumped into a single line.
  //         Month D HH:MM:SS.uuuuuu  <i2c_address  offset  len  page  bank  op>
  //         [data] steadyclock_ns
  // Returns a pair: header lines and number of log entries
  std::pair<size_t, size_t> dumpToFile();

  // Translate from a log file back to a vector of entries. Can be used
  // to replay the sequence of transactions or test the logging.
  static std::vector<I2cReplayEntry> loadFromLog(std::string logFile);

  // When Transceiver Information is updated (i.e. inserted), Update that
  // in the Header of the log file.
  // We will only be setting the info for the very last transceiver plugged
  // in to avoid potential memory leaks from higher layers.
  void setTcvrInfoInLog(
      const TransceiverManagementInterface& mgmtIf,
      const std::set<std::string>& portNames,
      const std::optional<FirmwareStatus>& status,
      const std::optional<Vendor>& vendor);

  static int getMaxNumberSlots() {
    return kMaxNumberBufferSlots;
  }

 private:
  // Buffer Config.
  const size_t size_;
  bool writeLog_{false};
  bool readLog_{false};
  bool disableOnFail_{false};

  std::vector<I2cLogEntry> buffer_;

  size_t head_{0};
  size_t tail_{0};
  size_t totalEntries_{0};
  std::string logFile_;
  mutable std::mutex mutex_;
  TransceiverManagementInterface mgmtIf_ =
      TransceiverManagementInterface::UNKNOWN;
  std::set<std::string> portNames_;
  std::optional<FirmwareStatus> fwStatus_;
  std::optional<Vendor> vendor_;

  size_t getSize() const {
    // Avoid the tsan errors. size_ is also a const member variable.
    std::lock_guard<std::mutex> g(mutex_);
    return size_;
  }

  void getEntryTime(std::stringstream& ss, const TimePointSystem& time_point);
  void getFieldName(std::stringstream& ss, const int field);

  template <typename T>
  void getOptional(std::stringstream& ss, T value);

  // Operations to re-construct I2cReplayEntry from a log file.
  static size_t getHeader(std::stringstream& ss, const I2cLogHeader& info);
  static std::string getField(const std::string& line, char left, char right);
  static TransceiverAccessParameter getParam(const std::string& str);
  static I2cLogBuffer::Operation getOp(const char op);
  static std::array<uint8_t, kMaxI2clogDataSize> getData(std::string str);
  static uint64_t getDelay(const std::string& str);
  static bool getSuccess(const std::string& str);
};

} // namespace facebook::fboss
