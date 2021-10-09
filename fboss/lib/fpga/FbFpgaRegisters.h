// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/logging/xlog.h>

#include <stdint.h>
#include <ostream>

namespace facebook::fboss {

enum class I2CRegisterType {
  DESC_UPPER,
  DESC_LOWER,
  RTC_STATUS,
};

struct I2CRegisterAddr {
  uint32_t baseAddr;
  uint32_t addrIncr;
};

class I2CRegisterAddrConstants {
 public:
  static I2CRegisterAddr getI2CRegisterAddr(int version, I2CRegisterType type);
};

template <class DataUnion>
struct I2cRegister {
  virtual ~I2cRegister() {}
  uint32_t getBaseAddr() {
    return addr_.baseAddr;
  }
  uint32_t getAddrIncr() {
    return addr_.addrIncr;
  }
  DataUnion dataUnion;

 protected:
  I2CRegisterAddr addr_;
};

union I2cDescriptorUpperDataUnion {
  uint32_t reg;
  struct __attribute__((packed)) {
    uint32_t offset : 8; // Register Address Byte.
    uint32_t page : 8;
    uint32_t bank : 8;
    uint32_t channel : 2;
    uint32_t reserved : 4;
    uint32_t i2cA2Access : 1; // Enables I2C Access from 0xA2 address (default
                              // 0xA0 access). Only for FPGA version
                              // >= 1.10, no-op for others
    uint32_t valid : 1; // Write 1 to trigger a new I2C txn.
  };
};

struct I2cDescriptorUpper : I2cRegister<I2cDescriptorUpperDataUnion> {
  explicit I2cDescriptorUpper(int version) {
    addr_ = I2CRegisterAddrConstants::getI2CRegisterAddr(
        version, I2CRegisterType::DESC_UPPER);
  }
};

inline std::ostream& operator<<(
    std::ostream& os,
    const I2cDescriptorUpper& upper) {
  auto data = upper.dataUnion;
  os << "I2cDescriptorUpper: offset=0x" << std::hex << data.offset << " page=0x"
     << data.page << " bank=0x" << data.bank << " channel=0x" << data.channel
     << " valid=0x" << data.valid << " i2cA2Access=0x" << data.i2cA2Access;
  return os;
}

union I2cDescriptorLowerDataUnion {
  uint32_t reg;
  struct __attribute__((packed)) {
    uint32_t len : 8; // Number of bytes will be accessed.
    uint32_t reserved : 20;
    uint32_t op : 2; // I2C read/write or flush command.
    uint32_t reserved2 : 2;
  };
};

struct I2cDescriptorLower : I2cRegister<I2cDescriptorLowerDataUnion> {
  explicit I2cDescriptorLower(int version) {
    addr_ = I2CRegisterAddrConstants::getI2CRegisterAddr(
        version, I2CRegisterType::DESC_LOWER);
  }
};

inline std::ostream& operator<<(
    std::ostream& os,
    const I2cDescriptorLower& lower) {
  auto data = lower.dataUnion;
  os << "I2cDescriptorLower: length=0x" << std::hex << data.len
     << " operation=0x" << data.op;
  return os;
}

union I2cRtcStatusDataUnion {
  uint32_t reg;
  struct __attribute__((packed)) {
    uint32_t desc0done : 1; // new txn or Write 1 to clear.
    uint32_t desc0error : 1; // Write 1 to clear.
    uint32_t reserved0 : 2;
    uint32_t desc1done : 1; // new txn or Write 1 to clear.
    uint32_t desc1error : 1; // Write 1 to clear.
    uint32_t reserved1 : 2;
    uint32_t desc2done : 1; // new txn or Write 1 to clear.
    uint32_t desc2error : 1; // Write 1 to clear.
    uint32_t reserved2 : 2;
    uint32_t desc3done : 1; // new txn or Write 1 to clear.
    uint32_t desc3error : 1; // Write 1 to clear.
    uint32_t reserved3 : 2;
    uint32_t reserved : 16;
  };
};

struct I2cRtcStatus : I2cRegister<I2cRtcStatusDataUnion> {
  explicit I2cRtcStatus(int version) {
    addr_ = I2CRegisterAddrConstants::getI2CRegisterAddr(
        version, I2CRegisterType::RTC_STATUS);
  }
};

inline std::ostream& operator<<(std::ostream& os, const I2cRtcStatus& status) {
  auto data = status.dataUnion;
  os << "RtcStatus: desc0done=0x" << std::hex << data.desc0done
     << " desc0error=0x" << data.desc0error << " desc1done=0x" << data.desc1done
     << " desc1error=0x" << data.desc1error << " desc2done=0x" << data.desc2done
     << " desc2error=0x" << data.desc2error << " desc3done=0x" << data.desc3done
     << " desc3error=0x" << data.desc3error;
  return os;
}

union MdioConfig {
  using addr = std::integral_constant<uint32_t, 0x0>;

  uint32_t reg;
  struct __attribute__((packed)) {
    uint32_t mdcFreq : 8;
    uint32_t fastMode : 1;
    uint32_t reserved1 : 22;
    uint32_t reset : 1;
  };
};

inline std::ostream& operator<<(std::ostream& os, const MdioConfig& config) {
  os << "MdioConfig: reset=0x" << std::hex << config.reset << " fastmode=0x"
     << config.fastMode << " mdcFreq=0x" << config.mdcFreq;
  return os;
}

union MdioCommand {
  using addr = std::integral_constant<uint32_t, 0x4>;

  uint32_t reg;
  struct __attribute__((packed)) {
    uint32_t devAddr : 5;
    uint32_t reserved1 : 2;
    uint32_t rw : 1;
    uint32_t phySel : 5;
    uint32_t reserved2 : 3;
    uint32_t regAddr : 16;
  };
};

inline std::ostream& operator<<(std::ostream& os, const MdioCommand& command) {
  os << "MdioCommand: devAddr=0x" << std::hex << command.devAddr
     << " rw=" << ((command.rw) ? "read" : "write") << " phySel=0x"
     << command.phySel << " regAddr=0x" << command.regAddr;
  return os;
}

union MdioWrite {
  using addr = std::integral_constant<uint32_t, 0x8>;

  uint32_t reg;
  struct __attribute__((packed)) {
    uint32_t data : 16;
    uint32_t reserved1 : 16;
  };
};

inline std::ostream& operator<<(std::ostream& os, const MdioWrite& write) {
  os << "MdioWrite: data=0x" << std::hex << write.data;
  return os;
}

union MdioRead {
  using addr = std::integral_constant<uint32_t, 0xc>;

  uint32_t reg;
  struct __attribute__((packed)) {
    uint32_t data : 16;
    uint32_t reserved1 : 16;
  };
};

inline std::ostream& operator<<(std::ostream& os, const MdioRead& read) {
  os << "MdioRead: data=0x" << std::hex << read.data;
  return os;
}

union MdioStatus {
  using addr = std::integral_constant<uint32_t, 0x10>;

  uint32_t reg;
  struct __attribute__((packed)) {
    uint32_t done : 1;
    uint32_t err : 1;
    uint32_t reserved1 : 6;
    uint32_t bitCnt : 6;
    uint32_t frameCnt : 1;
    uint32_t active : 1;
    uint32_t reserved2 : 16;
  };
};

inline std::ostream& operator<<(std::ostream& os, const MdioStatus& status) {
  os << "MdioStatus: done=0x" << std::hex << status.done << " err=0x"
     << status.err << " bitCnt=0x" << status.bitCnt << " frameCnt=0x"
     << status.frameCnt << " active=0x" << status.active;
  return os;
}

} // namespace facebook::fboss
