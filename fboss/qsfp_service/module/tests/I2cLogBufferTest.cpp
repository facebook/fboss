// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>

#include <folly/Random.h>
#include <folly/testing/TestUtil.h>

#include "fboss/lib/usb/TransceiverAccessParameter.h"
#include "fboss/qsfp_service/module/I2cLogBuffer.h"

namespace facebook::fboss {

namespace {
constexpr size_t kFullBuffer = 10;
} // namespace

class I2cLogBufferTest : public ::testing::Test {
 public:
  I2cLogBufferTest() : data_(kMaxI2clogDataSize), param_(10, 15, data_.size()) {
    folly::Random::DefaultGenerator seed(0xdeadbeef);
    folly::Random::seed(seed);
    for (auto& byte : data_) {
      byte = folly::Random::rand32() % 0xFF;
    }
    tmpDir_ = folly::test::TemporaryDirectory();
    kLogFile_ = tmpDir_.path().string() + "/logBufferTest.txt";
  }
  ~I2cLogBufferTest() override = default;
  void SetUp() override {}
  void TearDown() override {}

  std::string kLogFile_;
  folly::test::TemporaryDirectory tmpDir_;

  I2cLogBuffer createBuffer(
      const size_t size,
      bool read = true,
      bool write = true,
      bool disableOnFail = false) {
    cfg::TransceiverI2cLogging config;
    config.readLog() = read;
    config.writeLog() = write;
    config.disableOnFail() = disableOnFail;
    config.bufferSlots() = size;
    return I2cLogBuffer(config, kLogFile_);
  }

  std::vector<uint8_t> data_;
  TransceiverAccessParameter param_;
};

TEST_F(I2cLogBufferTest, basic) {
  // Create a buffer with a size of kFullBuffer.
  I2cLogBuffer logBuffer = createBuffer(kFullBuffer);

  // insert 1 element less
  const size_t kNumElements = kFullBuffer - 1;
  for (int i = 0; i < kNumElements; i++) {
    data_[0] = i;
    param_.i2cAddress = i;
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  }

  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  auto count = logBuffer.dump(entries);
  EXPECT_EQ(count.totalEntries, kNumElements);
  EXPECT_EQ(count.bufferEntries, kNumElements);
  EXPECT_EQ(entries.size(), kFullBuffer);
  for (int i = 0; i < kNumElements; i++) {
    EXPECT_EQ(entries[i].param.i2cAddress, i);
    EXPECT_EQ(entries[i].op, I2cLogBuffer::Operation::Read);
    EXPECT_EQ(entries[i].data[0], i);
  }
  // Once dumped, the logBuffer will be empty. Another dump
  // will have a count of 0.
  count = logBuffer.dump(entries);
  EXPECT_EQ(count.totalEntries, 0);
  EXPECT_EQ(count.bufferEntries, 0);
}

TEST_F(I2cLogBufferTest, basicFullBuffer) {
  I2cLogBuffer logBuffer = createBuffer(kFullBuffer);

  // insert kFullBuffer elements
  for (int i = 0; i < kFullBuffer; i++) {
    data_[0] = i;
    param_.i2cAddress = i;
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  }

  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  auto count = logBuffer.dump(entries);
  // total entries and entries available in log are the same
  EXPECT_EQ(count.totalEntries, kFullBuffer);
  EXPECT_EQ(count.bufferEntries, kFullBuffer);
  EXPECT_EQ(entries.size(), kFullBuffer);
  for (int i = 0; i < kFullBuffer; i++) {
    EXPECT_EQ(entries[i].param.i2cAddress, i);
    EXPECT_EQ(entries[i].op, I2cLogBuffer::Operation::Read);
    EXPECT_EQ(entries[i].data[0], i);
  }
  // Once dumped, the logBuffer will be empty. Another dump
  // will have a count of 0.
  count = logBuffer.dump(entries);
  EXPECT_EQ(count.totalEntries, 0);
  EXPECT_EQ(count.bufferEntries, 0);
}

TEST_F(I2cLogBufferTest, basicFullBufferAnd1Element) {
  I2cLogBuffer logBuffer = createBuffer(kFullBuffer);

  // insert kFullBuffer + 1 elements
  const size_t kNumElements = kFullBuffer + 1;
  for (int i = 0; i < kNumElements; i++) {
    data_[0] = i;
    param_.i2cAddress = i;
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  }

  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  auto count = logBuffer.dump(entries);
  EXPECT_EQ(count.totalEntries, kFullBuffer + 1);
  EXPECT_EQ(count.bufferEntries, kFullBuffer);
  EXPECT_EQ(entries.size(), kFullBuffer);
  for (int i = 0; i < kFullBuffer; i++) {
    EXPECT_EQ(entries[i].param.i2cAddress, i + 1);
    EXPECT_EQ(entries[i].op, I2cLogBuffer::Operation::Read);
    EXPECT_EQ(entries[i].data[0], i + 1);
  }
  // Once dumped, the logBuffer will be empty. Another dump
  // will have a count of 0.
  count = logBuffer.dump(entries);
  EXPECT_EQ(count.totalEntries, 0);
  EXPECT_EQ(count.bufferEntries, 0);
}

TEST_F(I2cLogBufferTest, testRange) {
  // Test Even and Odd Size LogBuffer
  const std::vector<size_t> bufferSizes = {1, 10, 11, 21, 128, 129};

  // Number of entries to test in each size:
  // How many elements we insert and test if the circular
  // buffer contains the right number of entries.
  // We will insert 1,2,.. up to 255 elements in the buffer
  const size_t maxNumInserts = 255;

  for (auto bufferSize : bufferSizes) {
    for (size_t inserts = 0; inserts < maxNumInserts; inserts++) {
      I2cLogBuffer logBuffer = createBuffer(bufferSize);

      // insert entry elements. Use the first byte of data and the
      // i2c address as a way to track what was inserted in the buffer
      // to make sure that we are tracking only the very last elements inserted
      // depending on the LogBuffer size.
      for (int i = 0; i < inserts; i++) {
        data_[0] = i;
        param_.i2cAddress = i;
        logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Write);
      }
      std::vector<I2cLogBuffer::I2cLogEntry> entries;

      // Expect number of inserts to match the total number of entries
      EXPECT_EQ(inserts, logBuffer.getTotalEntries());

      auto count = logBuffer.dump(entries);

      // Expect total number of entries to be 0 post dump
      EXPECT_EQ(0, logBuffer.getTotalEntries());

      // Expect that total logged elements is number of inserts
      EXPECT_EQ(count.totalEntries, inserts);
      // Expect that we have the following number of elements:
      //     minimum of the number of logged elements or LogBuffer size
      EXPECT_EQ(count.bufferEntries, std::min(inserts, bufferSize));
      // The updated vector size is always the size of the LogBuffer.
      EXPECT_EQ(entries.size(), bufferSize);
      // if the number of inserts is smaller than or equal to bufferSize:
      //   - the buffer should contain all the inserted elements
      // if the number of inserts is greater than bufferSize
      //   - we will only get the last entries in the buffer.
      int increment = (inserts <= bufferSize) ? 0 : inserts - bufferSize;
      for (int i = 0; i < std::min(bufferSize, inserts); i++) {
        EXPECT_EQ(entries[i].param.i2cAddress, i + increment);
        EXPECT_EQ(entries[i].op, I2cLogBuffer::Operation::Write);
        EXPECT_EQ(entries[i].data[0], i + increment);
      }

      // Once dumped, the logBuffer will be empty. Another dump
      // will have a count of 0.
      count = logBuffer.dump(entries);
      EXPECT_EQ(count.totalEntries, 0);
      EXPECT_EQ(count.bufferEntries, 0);
    }
  }
}

TEST_F(I2cLogBufferTest, testLargeData) {
  I2cLogBuffer logBuffer = createBuffer(kFullBuffer);

  std::vector<uint8_t> largeData(kMaxI2clogDataSize * 2);
  for (int i = 0; i < largeData.size(); i++) {
    largeData[i] = i;
  }
  TransceiverAccessParameter param(0, 0, largeData.size());

  // insert kFullBuffer elements
  for (int i = 0; i < kFullBuffer; i++) {
    logBuffer.log(param, largeData.data(), I2cLogBuffer::Operation::Read);
  }
  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  auto count = logBuffer.dump(entries);
  EXPECT_EQ(count.totalEntries, kFullBuffer);
  EXPECT_EQ(count.bufferEntries, kFullBuffer);
  EXPECT_EQ(entries.size(), kFullBuffer);

  for (int i = 0; i < kFullBuffer; i++) {
    // The data should be truncated to the size of the buffer slot.
    EXPECT_EQ(
        0,
        memcmp(
            entries[i].data.data(),
            largeData.data(),
            std::min(entries[i].data.size(), largeData.size())));
    EXPECT_GT(param.len, entries[i].data.size());
  }
}

TEST_F(I2cLogBufferTest, edgeCases) {
  // Size 0 is not allowed.
  EXPECT_THROW(createBuffer(0), std::invalid_argument);

  I2cLogBuffer logBuffer = createBuffer(kFullBuffer);
  // nullptr data
  EXPECT_THROW(
      logBuffer.log(param_, nullptr, I2cLogBuffer::Operation::Read),
      std::invalid_argument);
}

TEST_F(I2cLogBufferTest, testOnlyRead) {
  I2cLogBuffer logBuffer =
      createBuffer(kFullBuffer, /*read*/ true, /*write*/ false);
  // insert 3 elements Read
  for (int i = 0; i < 3; i++) {
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  }
  // insert 4 elements write (should not log)
  for (int i = 0; i < 4; i++) {
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Write);
  }
  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  auto count = logBuffer.dump(entries);
  EXPECT_EQ(count.totalEntries, 3);
  EXPECT_EQ(count.bufferEntries, 3);
}

TEST_F(I2cLogBufferTest, testOnlyWrite) {
  I2cLogBuffer logBuffer =
      createBuffer(kFullBuffer, /*read*/ false, /*write*/ true);
  // insert 3 elements Read (should not log)
  for (int i = 0; i < 3; i++) {
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  }
  // insert 4 elements write
  for (int i = 0; i < 4; i++) {
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Write);
  }
  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  auto count = logBuffer.dump(entries);
  EXPECT_EQ(count.totalEntries, 4);
  EXPECT_EQ(count.totalEntries, 4);
}

TEST_F(I2cLogBufferTest, testDisableOnFail) {
  I2cLogBuffer logBuffer = createBuffer(
      kFullBuffer, /*read*/ true, /*write*/ true, /*disableOnFail*/ true);
  // insert 1 elements Read
  logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  // insert 1 elements write
  logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Write);
  // Transaction failure resulting in a fail log.
  logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Write, false);
  // insert 1 element Read (should not log)
  logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  // insert 1 element write (should not log)
  logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Write);
  // check that we have 3 elements (including the one that failed).
  // since we have disableOnFail, we stop logging beyond error.
  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  auto count = logBuffer.dump(entries);
  EXPECT_EQ(count.totalEntries, 3);
  EXPECT_EQ(count.bufferEntries, 3);
}

TEST_F(I2cLogBufferTest, testNoDisableOnFail) {
  // check that if we dont have disable on fail that we contiue logging.
  I2cLogBuffer logBuffer = createBuffer(
      kFullBuffer, /*read*/ true, /*write*/ true, /*disableOnFail*/ false);
  // insert 1 elements Read
  logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  // insert 1 elements write
  logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Write);
  // Transaction failure resulting in a fail log.
  logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read, false);
  // insert 1 element Read
  logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  // insert 1 element write
  logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Write);
  // check that we have 5 elements since we dont disable logging on fail
  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  auto count = logBuffer.dump(entries);
  EXPECT_EQ(count.totalEntries, 5);
  EXPECT_EQ(count.bufferEntries, 5);
}

TEST_F(I2cLogBufferTest, testEmptyLogFile) {
  I2cLogBuffer logBuffer = createBuffer(kFullBuffer);

  auto ret = logBuffer.dumpToFile();

  int numberOfLines = 0;
  std::string line;
  std::ifstream myfile(kLogFile_);

  while (std::getline(myfile, line)) {
    ++numberOfLines;
  }
  EXPECT_EQ(ret.first, numberOfLines);
  EXPECT_EQ(ret.second, 0);
}

TEST_F(I2cLogBufferTest, testLogFile) {
  I2cLogBuffer logBuffer = createBuffer(kFullBuffer);

  // insert kFullBuffer elements
  for (int i = 0; i < kFullBuffer; i++) {
    data_[0] = i;
    param_.i2cAddress = i;
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  }

  auto ret = logBuffer.dumpToFile();

  int numberOfLines = 0;
  std::string line;
  std::ifstream myfile(kLogFile_);

  while (std::getline(myfile, line)) {
    ++numberOfLines;
  }

  EXPECT_EQ(ret.first + ret.second, numberOfLines);
}

TEST_F(I2cLogBufferTest, testReplayEmpty) {
  I2cLogBuffer logBuffer = createBuffer(kFullBuffer);
  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  logBuffer.dumpToFile();
  auto replayEntries = I2cLogBuffer::loadFromLog(kLogFile_);
  EXPECT_EQ(replayEntries.size(), 0);
}

TEST_F(I2cLogBufferTest, testReplayBasic) {
  I2cLogBuffer logBuffer = createBuffer(kFullBuffer);

  // insert kFullBuffer elements
  std::vector<std::array<uint8_t, kMaxI2clogDataSize>> allData(kFullBuffer);
  for (int i = 0; i < kFullBuffer; i++) {
    for (int j = 0; j < param_.len; j++) {
      // fill data
      allData[i][j] = i;
    }
    param_.i2cAddress = i;
    logBuffer.log(param_, allData[i].data(), I2cLogBuffer::Operation::Write);
  }

  logBuffer.dumpToFile();

  auto replayEntries = I2cLogBuffer::loadFromLog(kLogFile_);

  EXPECT_EQ(replayEntries.size(), kFullBuffer);

  for (int i = 0; i < kFullBuffer; i++) {
    EXPECT_EQ(replayEntries[i].param.i2cAddress, i);
    EXPECT_EQ(replayEntries[i].param.offset, param_.offset);
    EXPECT_EQ(replayEntries[i].param.len, param_.len);
    EXPECT_EQ(replayEntries[i].param.page, param_.page);
    EXPECT_EQ(replayEntries[i].param.bank, param_.bank);
    EXPECT_EQ(replayEntries[i].op, I2cLogBuffer::Operation::Write);
    for (int j = 0; j < replayEntries[i].param.len; j++) {
      EXPECT_EQ(replayEntries[i].data[j], allData[i][j]);
    }
  }
}

TEST_F(I2cLogBufferTest, testReplayScenarios) {
  I2cLogBuffer logBuffer = createBuffer(kFullBuffer);

  // insert kFullBuffer Logs
  std::vector<std::array<uint8_t, kMaxI2clogDataSize>> allData(kFullBuffer);
  std::array<TransceiverAccessParameter, kFullBuffer> allParam = {
      TransceiverAccessParameter(0, 0, 1),
      TransceiverAccessParameter(0, 0, 10),
      TransceiverAccessParameter(0, 10, 20),
      TransceiverAccessParameter(0, 10, 32),
      TransceiverAccessParameter(10, 0, 20),
      TransceiverAccessParameter(10, 0, 10),
      TransceiverAccessParameter(10, 10, 10),
      TransceiverAccessParameter(10, 10, 10),
      TransceiverAccessParameter(0, 0, 10, 10),
      TransceiverAccessParameter(10, 10, 10, 10),
  };
  std::array<I2cLogBuffer::Operation, kFullBuffer> allOps = {
      I2cLogBuffer::Operation::Read,
      I2cLogBuffer::Operation::Write,
      I2cLogBuffer::Operation::Write,
      I2cLogBuffer::Operation::Read,
      I2cLogBuffer::Operation::Read,
      I2cLogBuffer::Operation::Write,
      I2cLogBuffer::Operation::Read,
      I2cLogBuffer::Operation::Read,
      I2cLogBuffer::Operation::Write,
      I2cLogBuffer::Operation::Read,
  };

  auto lambda = [&]() {
    for (int i = 0; i < kFullBuffer; i++) {
      for (int j = 0; j < allParam[i].len; j++) {
        // fill data
        allData[i][j] = i;
      }
      // Make odd transactions fail, to check replay data will be identical.
      bool success = (i % 2 == 0) ? true : false;
      logBuffer.log(allParam[i], allData[i].data(), allOps[i], success);
    }

    logBuffer.dumpToFile();

    auto replayEntries = I2cLogBuffer::loadFromLog(kLogFile_);

    EXPECT_EQ(replayEntries.size(), kFullBuffer);

    for (int i = 0; i < kFullBuffer; i++) {
      EXPECT_EQ(replayEntries[i].param.i2cAddress, allParam[i].i2cAddress);
      EXPECT_EQ(replayEntries[i].param.offset, allParam[i].offset);
      EXPECT_EQ(replayEntries[i].param.len, allParam[i].len);
      EXPECT_EQ(replayEntries[i].param.page, allParam[i].page);
      EXPECT_EQ(replayEntries[i].param.bank, allParam[i].bank);
      EXPECT_EQ(replayEntries[i].op, allOps[i]);
      bool success = (i % 2 == 0) ? true : false;
      EXPECT_EQ(replayEntries[i].success, success);
      for (int j = 0; j < replayEntries[i].param.len; j++) {
        EXPECT_EQ(replayEntries[i].data[j], allData[i][j]);
      }
    }
  };

  // Run test lambda twice.
  lambda();
  lambda();
}

} // namespace facebook::fboss
