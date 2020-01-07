/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/PCA9548.h"
#include "fboss/lib/usb/CP2112.h"

#include <stdint.h>

namespace facebook::fboss {
void PCA9548::commit(uint8_t selector) {
  // The multiplexer address is shifted one to the left to
  // work with the underlying I2C libraries.
  VLOG(3) << "Selecting " << (int)selector << " on " << this;
  dev_->writeByte(address_ << 1, selector);
  selected_ = selector;
}

} // namespace facebook::fboss
