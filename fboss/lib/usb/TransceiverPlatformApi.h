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

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>

namespace facebook {
namespace fboss {

/*
 * Abstract class for handling the FPGA related API for QSFP transcever
 * modules. This class will be inherited by platform specific classes
 * which implement/overrides platform specific API for FPGA operations
 */
class TransceiverPlatformApi {
 public:
  TransceiverPlatformApi(){};
  virtual ~TransceiverPlatformApi(){};

  /* This function does a hard reset of the QSFP and this will be
   * called when port flap is seen on the port remains down.
   * This is a virtual function at this class and it needs to be
   * implemented by platform specific class like Minipack16QTransceiverApi
   * etc.
   */
  virtual void triggerQsfpHardReset(unsigned int module) = 0;
};

} // namespace fboss
} // namespace facebook
