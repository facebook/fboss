// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>

extern "C" {
#include <bcm/mpls.h>
}

namespace facebook::fboss {

struct BcmMplsTunnelSwitchImplT {
  bcm_mpls_tunnel_switch_t data;
};

struct BcmMplsTunnelSwitchT {
  BcmMplsTunnelSwitchT();
  ~BcmMplsTunnelSwitchT();
  BcmMplsTunnelSwitchImplT* get();

 private:
  std::unique_ptr<BcmMplsTunnelSwitchImplT> impl_;
};

} // namespace facebook::fboss
