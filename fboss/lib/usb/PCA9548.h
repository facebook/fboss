/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <glog/logging.h>
#include <stdint.h>

namespace facebook::fboss {
class CP2112Intf;

// The PCA9548 chip is a mux that allows internally connecting 0-8
// channels over i2c. Note that this is NOT thread safe and expects
// clients to only access the chip from a single thread.

class PCA9548 {
 public:
  static constexpr uint8_t WIDTH = 8;

  PCA9548(CP2112Intf* dev, uint8_t address) : dev_(dev), address_(address) {}

  void init(uint8_t selector = 0) {
    commit(selector);
  }

  // selectors
  void select(uint8_t selector) {
    commit(selector);
  }
  void selectOne(uint8_t channel) {
    CHECK_GT(WIDTH, channel);
    commit(static_cast<uint8_t>(1 << channel));
  }
  void selectAll() {
    commit(~0);
  }

  // unselection
  void unselect(uint8_t selector) {
    commit(selected_ & ~selector);
  }
  void unselectOne(uint8_t channel) {
    CHECK_GT(WIDTH, channel);
    commit(selected_ & ~(static_cast<uint8_t>(1 << channel)));
  }
  void unselectAll() {
    commit(0);
  }

  uint8_t selected() const {
    return selected_;
  }
  uint8_t isSelected(uint8_t channel) const {
    return selected_ & (1 << channel);
  }

  uint8_t address() const {
    return address_;
  }

 private:
  void commit(uint8_t selector);

  CP2112Intf* dev_{nullptr};

  // i2c address of the mux
  uint8_t address_{0};

  // Which channels are selected. The semantics here are that every
  // bit reperesents a channel and if 1, that channel is connected to
  // the i2c bus.
  uint8_t selected_{0};
};

} // namespace facebook::fboss
