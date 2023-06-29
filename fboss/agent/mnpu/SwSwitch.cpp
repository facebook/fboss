// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitch.h"

namespace facebook::fboss {

HwSwitch* SwSwitch::getHw_DEPRECATED() const {
  throw FbossError("get hw switch is unsupported in mnpu sw switch");
}

const Platform* SwSwitch::getPlatform_DEPRECATED() const {
  throw FbossError("get platform is unsupported in mnpu sw switch");
}

Platform* SwSwitch::getPlatform_DEPRECATED() {
  throw FbossError("get platform is unsupported in mnpu sw switch");
}

} // namespace facebook::fboss
