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

#include <vector>

namespace facebook::fboss {
class SwSwitch;
class TestEnsembleIf;
struct PortID;
class HwAsic;
struct SwitchID;
class PortDescriptor;

namespace utility {

void verifyCopp(SwSwitch* sw, SwitchID switchId, PortID port);
void verifySafeDiagCmds(TestEnsembleIf* ensemble);
void verifyLoadBalance(
    SwSwitch* sw,
    int ecmpWidth,
    const std::vector<PortDescriptor>& ecmpPorts);
void verifyDscpToQueueMapping(SwSwitch* sw, const std::vector<PortID>& ports);
} // namespace utility
} // namespace facebook::fboss
