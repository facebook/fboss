/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/rackmon/rack_firmware_update/UpdaterExceptions.h"
#include <iomanip>
#include <sstream>

namespace facebook::fboss::platform::rackmon {

std::string FirmwareStatusException::toHex(uint16_t val) {
  std::ostringstream oss;
  oss << std::hex << std::setfill('0') << std::setw(4) << val;
  return oss.str();
}

} // namespace facebook::fboss::platform::rackmon
