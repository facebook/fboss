// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>
#include <optional>

namespace facebook::fboss {

struct TransceiverAccessParameter {
  std::optional<uint8_t> i2cAddress;
  int offset;
  int len;
  std::optional<int> page;
  std::optional<int> bank;

  TransceiverAccessParameter(uint8_t i2cAddress_, int offset_, int len_)
      : i2cAddress(i2cAddress_), offset(offset_), len(len_) {}

  TransceiverAccessParameter(
      uint8_t i2cAddress_,
      int offset_,
      int len_,
      int page_)
      : i2cAddress(i2cAddress_), offset(offset_), len(len_), page(page_) {}

  // Addresses to be queried by external callers:
  enum : uint8_t {
    ADDR_QSFP = 0x50,
    ADDR_QSFP_A2 = 0x51,
  };
};

} // namespace facebook::fboss
