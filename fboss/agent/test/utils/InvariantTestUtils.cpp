/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/InvariantTestUtils.h"
#include "fboss/agent/gen-cpp2/validated_shell_commands_constants.h"
#include "fboss/agent/test/TestEnsembleIf.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"

namespace facebook::fboss::utility {

void verifyCopp(SwSwitch* sw, SwitchID switchId, PortID port) {
  utility::verifyCoppInvariantHelper(
      sw,
      switchId,
      sw->getHwAsicTable()->getHwAsicIf(switchId),
      sw->getState(),
      port);
}

void verifySafeDiagCmds(TestEnsembleIf* ensemble, const HwAsic* asic) {
  std::set<std::string> diagCmds;
  switch (asic->getAsicType()) {
    case cfg::AsicType::ASIC_TYPE_FAKE:
    case cfg::AsicType::ASIC_TYPE_MOCK:
    case cfg::AsicType::ASIC_TYPE_EBRO:
    case cfg::AsicType::ASIC_TYPE_GARONNE:
    case cfg::AsicType::ASIC_TYPE_YUBA:
    case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
    case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
    case cfg::AsicType::ASIC_TYPE_JERICHO2:
    case cfg::AsicType::ASIC_TYPE_JERICHO3:
    case cfg::AsicType::ASIC_TYPE_RAMON:
    case cfg::AsicType::ASIC_TYPE_RAMON3:
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
      break;

    case cfg::AsicType::ASIC_TYPE_TRIDENT2:
      diagCmds = validated_shell_commands_constants::TD2_TESTED_CMDS();
      break;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
      diagCmds = validated_shell_commands_constants::TH_TESTED_CMDS();
      break;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
      diagCmds = validated_shell_commands_constants::TH3_TESTED_CMDS();
      break;
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
      diagCmds = validated_shell_commands_constants::TH4_TESTED_CMDS();
      break;
  }
  if (diagCmds.size()) {
    for (auto i = 0; i < 10; ++i) {
      for (auto cmd : diagCmds) {
        std::string out;
        ensemble->runDiagCommand(cmd + "\n", out);
      }
    }
    std::string out;
    ensemble->runDiagCommand("quit\n", out);
  }
}

} // namespace facebook::fboss::utility
