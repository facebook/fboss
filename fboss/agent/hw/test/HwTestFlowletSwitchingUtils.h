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
class TestEnsembleIf;
}
namespace facebook::fboss::utility {

const int KMaxFlowsetTableSize = 32768;
const int kDlbEcmpMaxId = 200128;

bool validateFlowletSwitchingEnabled(
    const facebook::fboss::HwSwitch* hw,
    const cfg::FlowletSwitchingConfig& flowletCfg);

bool verifyEcmpForFlowletSwitching(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& routePrefix,
    const cfg::FlowletSwitchingConfig& flowletCfg,
    const cfg::PortFlowletConfig& portFlowletCfg,
    const bool flowletEnable);

bool verifyEcmpForNonFlowlet(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& routePrefix,
    const bool expectFlowsetFree);

bool validatePortFlowletQuality(
    const facebook::fboss::HwSwitch* hw,
    const PortID& portId,
    const cfg::PortFlowletConfig& portFlowletCfg,
    bool enable = true);

bool validateFlowletSwitchingDisabled(const facebook::fboss::HwSwitch* hw);

void setEcmpMemberStatus(const facebook::fboss::TestEnsembleIf* hw);

bool validateFlowSetTable(
    const facebook::fboss::HwSwitch* hw,
    const bool expectFlowsetSizeZero);

int getL3EcmpDlbFailPackets(const facebook::fboss::TestEnsembleIf* hw);

} // namespace facebook::fboss::utility
