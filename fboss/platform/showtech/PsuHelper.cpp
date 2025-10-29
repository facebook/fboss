// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/showtech/PsuHelper.h"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include <fmt/core.h>
#include <folly/String.h>

#include "fboss/lib/i2c/I2cDevIo.h"

namespace {
constexpr std::string_view kErrMsg = "Error: failed to read register `{}`: {}";

// Converts a 16-bit value in "Linear11" format to a standard double-precision
// floating-point number.
double linear11ToDecimal(uint16_t linear11) {
  int16_t exponent = (linear11 >> 11) & 0x1F;
  int16_t mantissa = linear11 & 0x7FF;
  if (exponent & 0x10) {
    exponent = exponent | 0xFFE0;
  }
  if (mantissa & 0x400) {
    mantissa = mantissa | 0xF800;
  }
  double value = mantissa * std::pow(2, exponent);
  return value;
}
} // namespace

namespace facebook::fboss::platform {

PsuHelper::PsuHelper(int busNum, int devAddr)
    : devAddr_(devAddr),
      i2cPsuIo_(
          std::make_unique<I2cDevIo>(
              fmt::format("/dev/i2c-{}", busNum),
              I2cIoType::I2cIoTypeRdWr)) {}

void PsuHelper::dumpRegisters() const {
  for (const auto& reg : PsuHelper::genericPsuRegisters_) {
    auto registerName = std::get<0>(reg);
    std::cout << fmt::format("{}: ", registerName);
    // Read register
    std::vector<uint8_t> readData;
    try {
      readData = readRegister(reg);
    } catch (const std::exception& e) {
      std::cout << fmt::format(kErrMsg, registerName, e.what()) << std::endl;
      continue;
    }
    // For READ_VOUT also read VOUT_MODE
    uint8_t voutModeValue{0};
    if (registerName == "READ_VOUT") {
      try {
        std::vector<uint8_t> voutMode{0};
        voutMode = readRegister(PsuHelper::voutModeRegister_);
        voutModeValue = (voutMode[0] & 0x10)
            ? (static_cast<int8_t>(voutMode[0] | 0xE0))
            : voutMode[0];
      } catch (const std::exception& e) {
        std::cout << fmt::format(
                         kErrMsg, std::get<0>(voutModeRegister_), e.what())
                  << std::endl;
        continue;
      }
    }
    // Print register data in human readable format
    auto valType = std::get<3>(reg);
    if (valType == ValueType::ASCII) {
      std::string data(readData.begin() + 1, readData.end());
      std::cout << folly::trimWhitespace(data).str() << std::endl;
    } else if (valType == ValueType::HEX && readData.size() == 1) {
      std::cout << std::hex << "0x" << std::setw(2) << std::setfill('0')
                << static_cast<int>(readData[0]) << std::endl;
    } else if (valType == ValueType::HEX && readData.size() == 2) {
      uint16_t combinedRegVal =
          (static_cast<uint16_t>(readData[0]) << 8) | readData[1];
      std::cout << std::hex << "0x" << std::setw(4) << std::setfill('0')
                << combinedRegVal << std::endl;
    } else if (valType == ValueType::LINEAR_11 && readData.size() == 2) {
      uint16_t combinedRegVal =
          (static_cast<uint16_t>(readData[1]) << 8) | readData[0];
      std::cout << linear11ToDecimal(combinedRegVal) << std::endl;
    } else if (valType == ValueType::LINEAR_16 && readData.size() == 2) {
      uint16_t combinedRegVal =
          (static_cast<uint16_t>(readData[1]) << 8) | readData[0];
      double data = static_cast<double>(combinedRegVal) *
          std::pow(2.0, static_cast<int8_t>(voutModeValue));
      std::cout << data << std::endl;
    } else {
      std::cout << fmt::format(kErrMsg, registerName, "unknown data format")
                << std::endl;
    }
  }
}

std::vector<uint8_t> PsuHelper::readRegister(
    const PsuHelper::PsuRegister& reg) const {
  auto [regName, offset, bytesToRead, valType] = reg;
  if (bytesToRead == 0) {
    // For ASCII registers, read the size byte first
    if (valType == PsuHelper::ValueType::ASCII) {
      uint8_t sizeByte = 0;
      i2cPsuIo_->read(devAddr_, offset, &sizeByte, 1);
      bytesToRead = sizeByte + 1;
    } else {
      throw std::invalid_argument(
          fmt::format("Invalid register '{}': read size of 0", regName));
    }
  }
  std::vector<uint8_t> buffer(bytesToRead);
  i2cPsuIo_->read(devAddr_, offset, buffer.data(), bytesToRead);
  return buffer;
}

} // namespace facebook::fboss::platform
