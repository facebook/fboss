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
  if (getAsic()->isSupported(HwAsic::Feature::P4_WARMBOOT)) {
#if defined(TAJO_SDK_VERSION_1_60_0)
    return std::unordered_map<std::string, std::string>{
        std::make_pair(SAI_KEY_EXT_DEVICE_ISSU_CAPABLE, "1")};
#endif
  }
  return std::unordered_map<std::string, std::string>();
}

} // namespace facebook::fboss
