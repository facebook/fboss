// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>

namespace facebook::fboss {

struct BcmMplsTunnelSwitchImplT;

struct BcmMplsTunnelSwitchT {
  BcmMplsTunnelSwitchT();
  ~BcmMplsTunnelSwitchT();
  BcmMplsTunnelSwitchImplT* get();

 private:
  std::unique_ptr<BcmMplsTunnelSwitchImplT> impl_;
};

} // namespace facebook::fboss
