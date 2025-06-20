/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/platforms/sai/SaiTajoPlatform.h"
#include "fboss/agent/hw/switch_asics/TajoAsic.h"

extern "C" {
#include <experimental/sai_attr_ext.h>
}
namespace facebook::fboss {

const std::unordered_map<std::string, std::string>
SaiTajoPlatform::getSaiProfileVendorExtensionValues() const {
  std::unordered_map<std::string, std::string> vendorExtensions;
  if (getAsic()->isSupported(HwAsic::Feature::P4_WARMBOOT)) {
#if defined(TAJO_P4_WB_SDK)
    vendorExtensions[SAI_KEY_EXT_DEVICE_ISSU_CAPABLE] = "1";
#endif
  }

  if (const char* basePath = getenv("BASE_OUTPUT_DIR")) {
    vendorExtensions["SAI_SDK_LOG_CONFIG_FILE"] =
        std::string(basePath) + "/res/config/sai_sdk_log_config.json";
  }
  return vendorExtensions;
}

} // namespace facebook::fboss
