/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestNeighborUtils.h"

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmHost.h"

extern "C" {
#include <bcm/l3.h>
}

namespace facebook::fboss::utility {
namespace {

bcm_l3_host_t getHost(int unit, const folly::IPAddress& ip) {
  bcm_l3_host_t host;
  bcm_l3_host_t_init(&host);
  if (ip.isV4()) {
    host.l3a_ip_addr = ip.asV4().toLongHBO();
  } else {
    host.l3a_flags |= BCM_L3_IP6;
    ipToBcmIp6(ip.asV6(), &(host.l3a_ip6_addr));
  }
  auto rv = bcm_l3_host_find(unit, &host);
  bcmCheckError(rv, "Unable to find host: ", ip);
  return host;
}
} // namespace

bool nbrExists(
    const HwSwitch* hwSwitch,
    InterfaceID intf,
    const folly::IPAddress& ip) {
  try {
    getHost(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip);

  } catch (const BcmError& e) {
    if (e.getBcmError() == BCM_E_NOT_FOUND) {
      return false;
    } else {
      throw;
    }
  }
  return true;
}

bool nbrProgrammedToCpu(
    const HwSwitch* hwSwitch,
    InterfaceID /*intf*/,
    const folly::IPAddress& ip) {
  auto host = getHost(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip);
  bcm_l3_egress_t egress;
  auto cpuFlags = (BCM_L3_L2TOCPU | BCM_L3_COPY_TO_CPU);
  bcm_l3_egress_t_init(&egress);
  bcm_l3_egress_get(0, host.l3a_intf, &egress);
  return (egress.flags & cpuFlags) == cpuFlags;
}

std::optional<uint32_t> getNbrClassId(
    const HwSwitch* hwSwitch,
    InterfaceID /*intf*/,
    const folly::IPAddress& ip) {
  auto host = getHost(static_cast<const BcmSwitch*>(hwSwitch)->getUnit(), ip);
  return host.l3a_lookup_class;
}
} // namespace facebook::fboss::utility
