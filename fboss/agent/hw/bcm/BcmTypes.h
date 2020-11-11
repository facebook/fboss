// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>

extern "C" {
#include <bcm/mpls.h>

#include <bcm/pkt.h>
#ifdef INCLUDE_PKTIO
#include <bcm/pktio.h>
#else
typedef void bcm_pktio_pkt_t;
#endif
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

struct BcmPacketT {
  bool usePktIO;
  union {
    bcm_pkt_t* pkt;
    bcm_pktio_pkt_t* pktioPkt;
  } ptrUnion;
};

} // namespace facebook::fboss
