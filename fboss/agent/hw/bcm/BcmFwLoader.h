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

#include <iostream>

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"

namespace facebook {
namespace fboss {

enum class FwType {
  LINKSCAN,
  QCM,
};

using BcmFwLoadFunc = std::function<
    void(const BcmSwitch*, const int core_id, const std::string& fwFile)>;
using BcmFwStopFunc = std::function<void(const BcmSwitch*, const int core_id)>;
using FwFileFunc = std::function<std::string()>;
using FwFileErrorHandler = std::function<void(const std::string& filePath)>;

/*
 *  The list of firmware images fw_images[] is defined in BcmFwLoader.cpp.
 *  The platform-specific micro core application  is in platform files.
 */
struct BcmFirmware {
  FwType fwType;
  // Append file path with SDK version if sdk_specific.  For example: -6.5.16
  bool sdkSpecific;
  int core_id;
  int addr;
  FwFileErrorHandler fwFileErrorHandler_;
  FwFileFunc getFwFile_;
  BcmFwLoadFunc bcmFwLoadFunc_;
  BcmFwStopFunc bcmFwStopFunc_;
};

class BcmFwLoader {
 public:
  static void loadFirmware(BcmSwitch* sw, HwAsic* hwAsic);
  static void stopFirmware(BcmSwitch* sw);

 private:
  static void loadFirmwareImpl(BcmSwitch* sw, FwType fwType);
  static void stopFirmwareImpl(BcmSwitch* sw, FwType fwType);
  static std::string getFwFile(const BcmFirmware& fw);
};

} // namespace fboss
} // namespace facebook
