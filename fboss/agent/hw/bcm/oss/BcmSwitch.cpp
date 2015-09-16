/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmSwitch.h"

extern "C" {
#include <opennsl/link.h>
#include <opennsl/error.h>
}

/*
 * Stubbed out.
 */
namespace facebook { namespace fboss {

void BcmSwitch::configureAdditionalEcmpHashSets() {}

void BcmSwitch::dropDhcpPackets() {}

void BcmSwitch::dropIPv6RAs() {}

void BcmSwitch::configureRxRateLimiting() {
  // OpenNSL doesn't yet provide functions for configuring rate-limiting,
  // so rate limiting settings must be baked into the binary driver.
}

void BcmSwitch::dumpState() const {}

void BcmSwitch::disableLinkscan() {
    // Disable the linkscan thread
  auto rv = opennsl_linkscan_enable_set(unit_, 0);
  CHECK(OPENNSL_SUCCESS(rv)) << "failed to stop BcmSwitch linkscan thread "
                             << opennsl_errmsg(rv);
}

int BcmSwitch::getHighresSamplers(
    HighresSamplerList* samplers,
    const folly::StringPiece namespaceString,
    const std::set<folly::StringPiece>& counterSet) {
  return 0;
}
}} //facebook::fboss
