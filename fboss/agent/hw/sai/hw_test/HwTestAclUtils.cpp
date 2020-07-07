/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestAclUtils.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/SwitchState.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {
int getAclTableNumAclEntries(const HwSwitch* hwSwitch) {
  const auto& aclTableManager = static_cast<const SaiSwitch*>(hwSwitch)
                                    ->managerTable()
                                    ->aclTableManager();
  auto aclTableId = aclTableManager.getAclTableHandle(SaiSwitch::kAclTable1)
                        ->aclTable->adapterKey();

  auto aclTableEntryListGot = SaiApiTable::getInstance()->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::EntryList());

  return aclTableEntryListGot.size();
}

bool numAclTableNumAclEntriesMatch(
    const HwSwitch* hwSwitch,
    int expectedNumAclEntries) {
  auto asicType = hwSwitch->getPlatform()->getAsic()->getAsicType();
  if (asicType == HwAsic::AsicType::ASIC_TYPE_TRIDENT2 ||
      asicType == HwAsic::AsicType::ASIC_TYPE_TOMAHAWK ||
      asicType == HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3) {
    // TODO(skhare)
    // Fix after CSP CS00010652266 is fixed.
    return true;
  }

  return utility::getAclTableNumAclEntries(hwSwitch) == expectedNumAclEntries;
}

void checkSwHwAclMatch(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName) {
  auto swAcl = state->getAcl(aclName);

  if (swAcl->getDscp()) {
    const auto& aclTableManager =
        static_cast<const SaiSwitch*>(hw)->managerTable()->aclTableManager();
    auto aclTableHandle =
        aclTableManager.getAclTableHandle(SaiSwitch::kAclTable1);
    auto aclEntryHandle =
        aclTableManager.getAclEntryHandle(aclTableHandle, aclName);
    auto aclEntryId = aclEntryHandle->aclEntry->adapterKey();
    auto aclFieldDscpGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDscp());
    auto [dscpVal, dscpMask] = aclFieldDscpGot.getDataAndMask();
    EXPECT_EQ(dscpVal, swAcl->getDscp().value());
    EXPECT_EQ(dscpMask, SaiAclTableManager::kDscpMask);
  }
}

bool isAclTableEnabled(const HwSwitch* hwSwitch) {
  const auto& aclTableManager = static_cast<const SaiSwitch*>(hwSwitch)
                                    ->managerTable()
                                    ->aclTableManager();
  auto aclTableHandle =
      aclTableManager.getAclTableHandle(SaiSwitch::kAclTable1);

  return aclTableHandle != nullptr;
}

template bool isQualifierPresent<cfg::IpFragMatch>(
    const HwSwitch* hwSwitch,
    const std::shared_ptr<SwitchState>& state,
    const std::string& aclName);

template <typename T>
bool isQualifierPresent(
    const HwSwitch* /*hwSwitch*/,
    const std::shared_ptr<SwitchState>& /*state*/,
    const std::string& /*aclName*/) {
  throw FbossError("Not implemented");
  return false;
}

} // namespace facebook::fboss::utility
