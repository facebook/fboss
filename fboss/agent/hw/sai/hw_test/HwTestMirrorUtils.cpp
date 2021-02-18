/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestMirrorUtils.h"
#include "fboss/agent/state/Mirror.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

void getAllMirrorDestinations(
    facebook::fboss::HwSwitch* /* hwSwitch */,
    std::vector<uint64_t>& /* destinations */) {}

uint32_t getMirrorPortIngressAndSflowFlags() {
  return 0;
}

uint32_t getMirrorPortIngressFlags() {
  return 0;
}

uint32_t getMirrorPortEgressFlags() {
  return 0;
}

uint32_t getMirrorPortSflowFlags() {
  return 0;
}

void verifyResolvedMirror(
    facebook::fboss::HwSwitch* /* hwSwitch */,
    const std::shared_ptr<facebook::fboss::Mirror>& /* mirror */) {}

void verifyUnResolvedMirror(
    facebook::fboss::HwSwitch* /* hwSwitch */,
    const std::shared_ptr<facebook::fboss::Mirror>& /* mirror */) {}

void verifyPortMirrorDestination(
    facebook::fboss::HwSwitch* /* hwSwitch */,
    PortID /* port */,
    uint32_t /* flags */,
    uint64_t /* mirror_dest_id */) {}

void verifyAclMirrorDestination(
    facebook::fboss::HwSwitch* /* hwSwitch */,
    const std::string& /* mirrorId */) {}

void verifyNoAclMirrorDestination(
    facebook::fboss::HwSwitch* /* hwSwitch */,
    const std::string& /* mirrorId */) {}

void verifyPortNoMirrorDestination(
    facebook::fboss::HwSwitch* /* hwSwitch */,
    PortID /* port */,
    uint32_t /* flags */) {}

bool isMirrorSflowTunnelEnabled(
    facebook::fboss::HwSwitch* /* hwSwitch */,
    uint64_t /* destination */) {
  return false;
}

HwResourceStats getHwTableStats(facebook::fboss::HwSwitch* /* hwSwitch */) {
  return HwResourceStats();
}

} // namespace facebook::fboss::utility
