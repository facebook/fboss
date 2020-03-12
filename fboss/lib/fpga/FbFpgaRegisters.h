// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/logging/xlog.h>

#include <stdint.h>
#include <ostream>

namespace facebook::fboss {
union I2cDescriptorUpper {
  using baseAddr = std::integral_constant<uint32_t, 0x504>;
  using addrIncr = std::integral_constant<uint32_t, 0x20>;

  uint32_t reg;
  struct __attribute__((packed)) {
    uint32_t offset : 8; // Register Address Byte.
    uint32_t page : 8;
    uint32_t bank : 8;
    uint32_t channel : 2;
    uint32_t reserved : 5;
    uint32_t valid : 1; // Write 1 to trigger a new I2C txn.
  };
};

inline std::ostream& operator<<(
    std::ostream& os,
    const I2cDescriptorUpper& upper) {
  os << "I2cDescriptorUpper: offset=0x" << std::hex << upper.offset
     << " page=0x" << upper.page << " bank=0x" << upper.bank << " channel=0x"
     << upper.channel << " valid=0x" << upper.valid;
  return os;
}

union I2cDescriptorLower {
  using baseAddr = std::integral_constant<uint32_t, 0x500>;
  using addrIncr = std::integral_constant<uint32_t, 0x20>;

  uint32_t reg;
  struct __attribute__((packed)) {
    uint32_t len : 8; // Number of bytes will be accessed.
    uint32_t reserved : 20;
    uint32_t op : 2; // I2C read/write or flush command.
    uint32_t reserved2 : 2;
  };
};

inline std::ostream& operator<<(
    std::ostream& os,
    const I2cDescriptorLower& lower) {
  os << "I2cDescriptorLower: length=0x" << std::hex << lower.len
     << " operation=0x" << lower.op;
  return os;
}

union I2cRtcStatus {
  using baseAddr = std::integral_constant<uint32_t, 0x600>;
  using addrIncr = std::integral_constant<uint32_t, 0x4>;

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

inline std::ostream& operator<<(std::ostream& os, const I2cRtcStatus& status) {
  os << "RtcStatus: desc0done=0x" << std::hex << status.desc0done
     << " desc0error=0x" << status.desc0error << " desc1done=0x"
     << status.desc1done << " desc1error=0x" << status.desc1error
     << " desc2done=0x" << status.desc2done << " desc2error=0x"
     << status.desc2error << " desc3done=0x" << status.desc3done
     << " desc3error=0x" << status.desc3error;
  return os;
}

union MdioConfig {
  using addr = std::integral_constant<uint32_t, 0x200>;

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
  using addr = std::integral_constant<uint32_t, 0x204>;

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
  using addr = std::integral_constant<uint32_t, 0x208>;

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
  using addr = std::integral_constant<uint32_t, 0x20c>;

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
  using addr = std::integral_constant<uint32_t, 0x210>;

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
