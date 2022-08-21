/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"

#include "fboss/agent/hw/bcm/BcmExactMatchUtils.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTeFlowTable.h"

namespace facebook::fboss::utility {

bool validateTeFlowGroupEnabled(const facebook::fboss::HwSwitch* hw) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);

  int gid = bcmSwitch->getPlatform()->getAsic()->getDefaultTeFlowGroupID();
  return fpGroupExists(bcmSwitch->getUnit(), getEMGroupID(gid)) &&
      validateDstIpHint(
             bcmSwitch->getUnit(),
             bcmSwitch->getTeFlowTable()->getHintId(),
             getEmDstIpHintStartBit(),
             getEmDstIpHintEndBit());
}

} // namespace facebook::fboss::utility
