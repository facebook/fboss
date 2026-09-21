// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

namespace facebook::fboss {

class I2cDevIoError : public std::exception {
 public:
  explicit I2cDevIoError(const std::string& what) : what_(what) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

 private:
  std::string what_;
};

class I2cDevImplError : public I2cDevIoError {
 public:
  explicit I2cDevImplError(const std::string& what) : I2cDevIoError(what) {}
};

} // namespace facebook::fboss
