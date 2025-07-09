// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include "ModbusError.h"
#include "Msg.h"

namespace rackmon {

struct BadResponseError : public std::runtime_error {
  BadResponseError(const std::string& field, uint32_t exp, uint32_t val)
      : std::runtime_error(
            "Bad Response Received FIELD:" + field + " Expected: " +
            std::to_string(exp) + " Got: " + std::to_string(val)) {}
};

// ---- Generic Request/Response --------
struct Request : public Msg {
  uint8_t& reqFunction = raw[1];
};

struct Response : public Msg {
  uint8_t& respFunction = raw[1];

 protected:
  void
  checkValue(const std::string& what, uint32_t value, uint32_t expectedValue);
  void decode() override;
};

//---------- Read Holding Registers -------

struct ReadHoldingRegistersReq : public Request {
 private:
  static constexpr uint8_t kExpectedFunction = 0x3;
  uint8_t deviceAddr_ = 0;
  uint16_t registerOffset_ = 0;
  uint16_t registerCount_ = 0;

  void encode() override;

 public:
  ReadHoldingRegistersReq(
      uint8_t deviceAddr,
      uint16_t registerOffset,
      uint16_t registerCount);
  ReadHoldingRegistersReq() = default;
};

struct ReadHoldingRegistersResp : public Response {
 private:
  static constexpr uint8_t kExpectedFunction = 0x3;
  uint8_t expectedDeviceAddr_ = 0;
  std::vector<uint16_t>& regs_;

  void decode() override;

 public:
  explicit ReadHoldingRegistersResp(
      uint8_t deviceAddr,
      std::vector<uint16_t>& regs);
};

//----------- Write Single Register -------
struct WriteSingleRegisterReq : public Request {
 private:
  static constexpr uint8_t kExpectedFunction = 0x6;
  uint8_t deviceAddr_ = 0;
  uint16_t registerOffset_ = 0;
  uint16_t value_ = 0;

  void encode() override;

 public:
  WriteSingleRegisterReq(
      uint8_t deviceAddr,
      uint16_t registerOffset,
      uint16_t value);
  WriteSingleRegisterReq() = default;
};

struct WriteSingleRegisterResp : public Response {
 private:
  static constexpr uint8_t kExpectedFunction = 0x6;
  uint8_t expectedDeviceAddr_ = 0;
  uint16_t expectedRegisterOffset_ = 0;
  uint16_t value_ = 0;
  std::optional<uint16_t> expectedValue_{};

  void decode() override;

 public:
  WriteSingleRegisterResp(uint8_t deviceAddr, uint16_t registerOffset);
  WriteSingleRegisterResp(
      uint8_t deviceAddr,
      uint16_t registerOffset,
      uint16_t expectedValue);
  WriteSingleRegisterResp() = default;
  uint16_t writtenValue() const {
    return value_;
  }
};

//----------- Write Multiple Registers ------

// Users are expected to use the << operator to
// push in the register contents, helps them
// take advantage of any endian-conversions privided
// by Msg.h
struct WriteMultipleRegistersReq : public Request {
 private:
  static constexpr uint8_t kExpectedFunction = 0x10;
  uint8_t deviceAddr_ = 0;
  uint16_t registerOffset_ = 0;

  void encode() override;

 public:
  WriteMultipleRegistersReq(uint8_t deviceAddr, uint16_t registerOffset);
  WriteMultipleRegistersReq() = default;
};

struct WriteMultipleRegistersResp : public Response {
 private:
  static constexpr uint8_t kExpectedFunction = 0x10;
  uint8_t expectedDeviceAddr_ = 0;
  uint16_t expectedRegisterOffset_ = 0;
  uint16_t expectedRegisterCount_ = 0;

  void decode() override;

 public:
  WriteMultipleRegistersResp(
      uint8_t deviceAddr,
      uint16_t registerOffset,
      uint16_t registerCount);
  WriteMultipleRegistersResp() = default;
};

//---------- Read File Record ----------------
struct FileRecord {
  uint16_t fileNum = 0;
  uint16_t recordNum = 0;
  std::vector<uint16_t> data{};
  FileRecord() = default;
  explicit FileRecord(size_t num) : data(num) {}
  explicit FileRecord(uint16_t file, uint16_t record, size_t num)
      : fileNum(file), recordNum(record), data(num) {}
};
void from_json(const nlohmann::json& j, FileRecord& file);
void to_json(nlohmann::json& j, const FileRecord& file);

struct ReadFileRecordReq : public Request {
 private:
  static constexpr uint8_t kExpectedFunction = 0x14;
  static constexpr uint8_t kReferenceType = 0x6;
  uint8_t deviceAddr_ = 0;
  const std::vector<FileRecord>& records_;

  void encode() override;

 public:
  ReadFileRecordReq(uint8_t deviceAddr, const std::vector<FileRecord>& records);
};

struct ReadFileRecordResp : public Response {
 private:
  static constexpr uint8_t kExpectedFunction = 0x14;
  static constexpr uint8_t kReferenceType = 0x6;
  uint8_t deviceAddr_ = 0;
  std::vector<FileRecord>& records_;

  void decode() override;

 public:
  ReadFileRecordResp(uint8_t deviceAddr, std::vector<FileRecord>& records);
};

} // namespace rackmon
