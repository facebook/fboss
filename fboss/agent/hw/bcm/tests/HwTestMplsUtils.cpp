/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestMplsUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"

extern "C" {
#include <bcm/l3.h>
#include <bcm/mpls.h>
}

namespace facebook::fboss::utility {

int getLabelSwappedWithForTopLabel(uint32_t label) {
  bcm_mpls_tunnel_switch_t info;
  bcm_mpls_tunnel_switch_t_init(&info);
  info.label = label;
  info.port = BCM_GPORT_INVALID;
  auto rv = bcm_mpls_tunnel_switch_get(0, &info); // query label fib
  bcmCheckError(rv, "getLabelSwappedWithForTopLabel failed to query label fib");
  bcm_l3_egress_t egr;
  bcm_l3_egress_t_init(&egr);
  rv = bcm_l3_egress_get(0, info.egress_if, &egr); // query egress
  bcmCheckError(
      rv, "getLabelSwappedWithForTopLabel failed to query egress interface");
  return egr.mpls_label;
}

} // namespace facebook::fboss::utility
