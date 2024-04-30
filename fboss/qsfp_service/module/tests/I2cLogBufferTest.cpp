// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include <vector>

#include "fboss/lib/usb/TransceiverAccessParameter.h"
#include "fboss/qsfp_service/module/I2cLogBuffer.h"

namespace facebook::fboss {

constexpr size_t kFullBuffer = 10;

class I2cLogBufferTest : public ::testing::Test {
 public:
  I2cLogBufferTest() : data_(kMaxI2clogDataSize), param_(0, 0, data_.size()) {
    std::fill(data_.begin(), data_.end(), 0);
  }
  ~I2cLogBufferTest() override = default;
  void SetUp() override {}
  void TearDown() override {}
  std::vector<uint8_t> data_;
  TransceiverAccessParameter param_;
};

TEST_F(I2cLogBufferTest, basic) {
  // Create a buffer with a size of kFullBuffer.
  I2cLogBuffer logBuffer(kFullBuffer);

  // insert 1 element less
  const size_t kNumElements = kFullBuffer - 1;
  for (int i = 0; i < kNumElements; i++) {
    data_[0] = i;
    param_.i2cAddress = i;
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  }

  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  size_t count = logBuffer.dump(entries);
  EXPECT_EQ(count, kNumElements);
  EXPECT_EQ(entries.size(), kFullBuffer);
  for (int i = 0; i < kNumElements; i++) {
    EXPECT_EQ(entries[i].param.i2cAddress, i);
    EXPECT_EQ(entries[i].op, I2cLogBuffer::Operation::Read);
    EXPECT_EQ(entries[i].data[0], i);
  }
  // Once dumped, the logBuffer will be empty. Another dump
  // will have a count of 0.
  EXPECT_EQ(logBuffer.dump(entries), 0);
}

TEST_F(I2cLogBufferTest, basicFullBuffer) {
  I2cLogBuffer logBuffer(kFullBuffer);

  // insert kFullBuffer elements
  for (int i = 0; i < kFullBuffer; i++) {
    data_[0] = i;
    param_.i2cAddress = i;
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  }

  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  size_t count = logBuffer.dump(entries);
  EXPECT_EQ(count, kFullBuffer);
  EXPECT_EQ(entries.size(), kFullBuffer);
  for (int i = 0; i < kFullBuffer; i++) {
    EXPECT_EQ(entries[i].param.i2cAddress, i);
    EXPECT_EQ(entries[i].op, I2cLogBuffer::Operation::Read);
    EXPECT_EQ(entries[i].data[0], i);
  }
  // Once dumped, the logBuffer will be empty. Another dump
  // will have a count of 0.
  EXPECT_EQ(logBuffer.dump(entries), 0);
}

TEST_F(I2cLogBufferTest, basicFullBufferAnd1Element) {
  I2cLogBuffer logBuffer(kFullBuffer);

  // insert kFullBuffer + 1 elements
  const size_t kNumElements = kFullBuffer + 1;
  for (int i = 0; i < kNumElements; i++) {
    data_[0] = i;
    param_.i2cAddress = i;
    logBuffer.log(param_, data_.data(), I2cLogBuffer::Operation::Read);
  }

  std::vector<I2cLogBuffer::I2cLogEntry> entries;
  size_t count = logBuffer.dump(entries);
  EXPECT_EQ(count, kFullBuffer);
  EXPECT_EQ(entries.size(), kFullBuffer);
  for (int i = 0; i < kFullBuffer; i++) {
    EXPECT_EQ(entries[i].param.i2cAddress, i + 1);
    EXPECT_EQ(entries[i].op, I2cLogBuffer::Operation::Read);
    EXPECT_EQ(entries[i].data[0], i + 1);
  }
  // Once dumped, the logBuffer will be empty. Another dump
  // will have a count of 0.
  EXPECT_EQ(logBuffer.dump(entries), 0);
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
      I2cLogBuffer logBuffer(bufferSize);

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

      size_t count = logBuffer.dump(entries);

      // Expect total number of entries to be 0 post dump
      EXPECT_EQ(0, logBuffer.getTotalEntries());

      // Expect that we have the following number of elements:
      //     minimum of the number of logged elements or LogBuffer size
      EXPECT_EQ(count, std::min(inserts, bufferSize));
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
      EXPECT_EQ(logBuffer.dump(entries), 0);
    }
  }
}

TEST_F(I2cLogBufferTest, testLargeData) {
  I2cLogBuffer logBuffer(kFullBuffer);

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
  size_t count = logBuffer.dump(entries);
  EXPECT_EQ(count, kFullBuffer);
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
  EXPECT_THROW(I2cLogBuffer logBuffer(0), std::invalid_argument);

  I2cLogBuffer logBuffer(kFullBuffer);
  // nullptr data
  EXPECT_THROW(
      logBuffer.log(param_, nullptr, I2cLogBuffer::Operation::Read),
      std::invalid_argument);
}

} // namespace facebook::fboss
