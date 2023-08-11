// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>

namespace rackmon {

struct TimeoutException : public std::runtime_error {
  TimeoutException() : std::runtime_error("Timeout") {}
};

class Device {
 protected:
  const std::string device_;
  int deviceFd_ = -1;
  int waitTimePerByteMs = -1;

  // Wait for read for provided timeoutMs, returns
  // remaining time in ms.
  virtual int waitRead(int timeoutMs);
  virtual void waitWrite() {}

 public:
  explicit Device(const std::string& device) : device_(device) {}
  virtual ~Device() {}
  virtual void open();
  virtual void close();
  virtual void write(const uint8_t* buf, size_t len);
  virtual void ioctl(unsigned long cmd, void* data);
  virtual size_t read(uint8_t* buf, size_t exactLen, int timeoutMs);
  virtual bool exists();
};
} // namespace rackmon
