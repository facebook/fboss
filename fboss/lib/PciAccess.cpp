/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/PciAccess.h"

#include <fstream>
#include <string>

namespace {
constexpr uint8_t kMemSpaceAccess = 0x42;
}

namespace facebook::fboss {
PciAccess::PciAccess(std::string devicePath) : devicePath_(devicePath) {}

PciAccess::~PciAccess() {}

void PciAccess::enableMemSpaceAccess() {
  std::string configPath(devicePath_);
  configPath.append("/config");
  std::fstream confFile(
      configPath.c_str(), std::fstream::binary | std::fstream::out);
  confFile.seekp(4, confFile.beg);
  confFile.put((char)kMemSpaceAccess);
  confFile.flush();
  confFile.close();
}

} // namespace facebook::fboss
