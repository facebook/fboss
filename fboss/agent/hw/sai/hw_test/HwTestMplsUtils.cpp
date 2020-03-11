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
#include "fboss/agent/hw/sai/api/MplsApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include <folly/gen/Base.h>

namespace facebook::fboss::utility {

/*
 * Retrieve the next label swapped with the given the label entry
 */
int getLabelSwappedWithForTopLabel(const HwSwitch* hwSwitch, uint32_t label) {
  auto switchId = static_cast<const facebook::fboss::SaiSwitch*>(hwSwitch)
                      ->managerTable()
                      ->switchManager()
                      .getSwitchSaiId();
  auto& mplsApi = SaiApiTable::getInstance()->mplsApi();
  SaiInSegTraits::InSegEntry is{switchId, label};
  auto nextHopId =
      mplsApi.getAttribute(is, SaiInSegTraits::Attributes::NextHopId());
  auto& nextHopApi = SaiApiTable::getInstance()->nextHopApi();
  auto labelStack = nextHopApi.getAttribute(
      NextHopSaiId(nextHopId), SaiMplsNextHopTraits::Attributes::LabelStack{});
  if (labelStack.size() != 1) {
    throw FbossError(
        "getLabelSwappedWithForTopLabel to retrieve label stack to swap with.");
  }
  return labelStack.back();
}

} // namespace facebook::fboss::utility
