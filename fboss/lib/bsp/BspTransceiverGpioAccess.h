// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <gpiod.h>
#include "fboss/lib/bsp/BspTransceiverAccessImpl.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

class BspTransceiverGpioAccessError : public std::exception {
 public:
  explicit BspTransceiverGpioAccessError(const std::string& what)
      : what_(what) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

 private:
  std::string what_;
};

namespace facebook {
namespace fboss {

class BspTransceiverGpioAccess : public BspTransceiverAccessImpl {
 public:
  BspTransceiverGpioAccess(uint32_t tcvr, BspTransceiverMapping& tcvrMapping);
  ~BspTransceiverGpioAccess() override;

  bool isPresent() override;
  void init(bool force) override;
  void holdReset() override;
  void releaseReset() override;

 private:
  struct gpiod_chip* chip_{nullptr};
};

} // namespace fboss
} // namespace facebook
