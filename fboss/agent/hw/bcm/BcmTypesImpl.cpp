// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmTypes.h"

namespace facebook::fboss {

BcmMplsTunnelSwitchT::BcmMplsTunnelSwitchT()
    : impl_(std::make_unique<BcmMplsTunnelSwitchImplT>()) {}

BcmMplsTunnelSwitchT::~BcmMplsTunnelSwitchT() {}

BcmMplsTunnelSwitchImplT* BcmMplsTunnelSwitchT::get() {
  return impl_.get();
}

} // namespace facebook::fboss
