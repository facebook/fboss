// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/Format.h>
#include <folly/io/async/EventBase.h>
#include "fboss/agent/Utils.h"

namespace facebook::fboss {

template <class I2cControllerT>
class I2cControllerWithEvb {
 public:
  I2cControllerWithEvb(I2cControllerT* controller)
      : i2cController_(controller) {
    th_ = std::make_unique<std::thread>();
  }

  void start(uint32_t pim) {
    th_.reset(new std::thread([&, pim]() {
      initThread(folly::format("I2c_pim{:d}_ctrl0", pim).str());
      evb_.loopForever();
    }));
  }

  void stop(uint32_t pim) {
    evb_.runInEventBaseThread([&] { evb_.terminateLoopSoon(); });
    th_->join();
  }

  I2cControllerT* i2cController_;
  folly::EventBase evb_;
  std::unique_ptr<std::thread> th_;
};

} // namespace facebook::fboss
