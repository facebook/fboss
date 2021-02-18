/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/HwSwitch.h"

namespace facebook::fboss {
class Mirror;

namespace utility {
void verifyResolvedMirror(
    facebook::fboss::HwSwitch* hwSwitch,
    const std::shared_ptr<facebook::fboss::Mirror>& mirror);
void verifyUnResolvedMirror(
    facebook::fboss::HwSwitch* hwSwitch,
    const std::shared_ptr<facebook::fboss::Mirror>& mirror);
void verifyPortMirrorDestination(
    facebook::fboss::HwSwitch* hwSwitch,
    PortID port,
    uint32_t flags,
    uint64_t mirror_dest_id);
void verifyPortNoMirrorDestination(
    facebook::fboss::HwSwitch* hwSwitch,
    PortID port,
    uint32_t flags);
void verifyAclMirrorDestination(
    facebook::fboss::HwSwitch* hwSwitch,
    const std::string& mirrorId);
void verifyNoAclMirrorDestination(
    facebook::fboss::HwSwitch* hwSwitch,
    const std::string& mirrorId);
uint32_t getMirrorPortIngressAndSflowFlags();
uint32_t getMirrorPortIngressFlags();
uint32_t getMirrorPortEgressFlags();
uint32_t getMirrorPortSflowFlags();
void getAllMirrorDestinations(
    facebook::fboss::HwSwitch* hwSwitch,
    std::vector<uint64_t>& destinations);
bool isMirrorSflowTunnelEnabled(
    facebook::fboss::HwSwitch* hwSwitch,
    uint64_t destination);
HwResourceStats getHwTableStats(facebook::fboss::HwSwitch* hwSwitch);
} // namespace utility
} // namespace facebook::fboss
