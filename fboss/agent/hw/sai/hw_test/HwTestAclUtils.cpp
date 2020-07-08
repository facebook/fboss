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

  const auto& aclTableManager =
      static_cast<const SaiSwitch*>(hw)->managerTable()->aclTableManager();
  auto aclTableHandle =
      aclTableManager.getAclTableHandle(SaiSwitch::kAclTable1);
  auto aclEntryHandle =
      aclTableManager.getAclEntryHandle(aclTableHandle, aclName);
  auto aclEntryId = aclEntryHandle->aclEntry->adapterKey();

  auto aclFieldPriorityExpected =
      aclTableManager.swPriorityToSaiPriority(swAcl->getPriority());
  auto aclFieldPriorityGot = SaiApiTable::getInstance()->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::Priority());
  EXPECT_EQ(aclFieldPriorityGot, aclFieldPriorityExpected);

  if (swAcl->getSrcIp().first && swAcl->getSrcIp().first.isV6()) {
    auto aclFieldSrcIpV6Got = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldSrcIpV6());
    auto [srcIpV6DataGot, srcIpV6MaskGot] = aclFieldSrcIpV6Got.getDataAndMask();
    auto srcIpV6MaskExpected = folly::IPAddressV6(
        folly::IPAddressV6::fetchMask(swAcl->getSrcIp().second));

    EXPECT_EQ(srcIpV6DataGot, swAcl->getSrcIp().first.asV6());
    EXPECT_EQ(srcIpV6MaskGot, srcIpV6MaskExpected);
  }

  if (swAcl->getDstIp().first && swAcl->getDstIp().first.isV6()) {
    auto aclFieldDstIpV6Got = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDstIpV6());
    auto [dstIpV6DataGot, dstIpV6MaskGot] = aclFieldDstIpV6Got.getDataAndMask();
    auto dstIpV6MaskExpected = folly::IPAddressV6(
        folly::IPAddressV6::fetchMask(swAcl->getDstIp().second));

    EXPECT_EQ(dstIpV6DataGot, swAcl->getDstIp().first.asV6());
    EXPECT_EQ(dstIpV6MaskGot, dstIpV6MaskExpected);
  }

  if (swAcl->getL4SrcPort()) {
    auto aclFieldL4SrcPortGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldL4SrcPort());
    auto [l4SrcPortDataGot, l4SrcPortMaskGot] =
        aclFieldL4SrcPortGot.getDataAndMask();
    EXPECT_EQ(l4SrcPortDataGot, swAcl->getL4SrcPort().value());
    EXPECT_EQ(l4SrcPortMaskGot, SaiAclTableManager::kL4PortMask);
  }

  if (swAcl->getL4DstPort()) {
    auto aclFieldL4DstPortGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldL4DstPort());
    auto [l4DstPortDataGot, l4DstPortMaskGot] =
        aclFieldL4DstPortGot.getDataAndMask();
    EXPECT_EQ(l4DstPortDataGot, swAcl->getL4DstPort().value());
    EXPECT_EQ(l4DstPortMaskGot, SaiAclTableManager::kL4PortMask);
  }

  if (swAcl->getProto()) {
    auto aclFieldIpProtocolGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldIpProtocol());
    auto [ipProtocolDataGot, ipProtocolMaskGot] =
        aclFieldIpProtocolGot.getDataAndMask();
    EXPECT_EQ(ipProtocolDataGot, swAcl->getProto().value());
    EXPECT_EQ(ipProtocolMaskGot, SaiAclTableManager::kIpProtocolMask);
  }

  if (swAcl->getTcpFlagsBitMap()) {
    auto aclFieldTcpFlagsGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldTcpFlags());
    auto [tcpFlagsDataGot, tcpFlagsMaskGot] =
        aclFieldTcpFlagsGot.getDataAndMask();
    EXPECT_EQ(tcpFlagsDataGot, swAcl->getTcpFlagsBitMap().value());
    EXPECT_EQ(tcpFlagsMaskGot, SaiAclTableManager::kTcpFlagsMask);
  }

  if (swAcl->getDscp()) {
    auto aclFieldDscpGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDscp());
    auto [dscpVal, dscpMask] = aclFieldDscpGot.getDataAndMask();
    EXPECT_EQ(dscpVal, swAcl->getDscp().value());
    EXPECT_EQ(dscpMask, SaiAclTableManager::kDscpMask);
  }

  if (swAcl->getTtl()) {
    auto aclFieldTtlGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldTtl());
    auto [ttlVal, ttlMask] = aclFieldTtlGot.getDataAndMask();
    EXPECT_EQ(ttlVal, swAcl->getTtl().value().getValue());
    EXPECT_EQ(ttlMask, swAcl->getTtl().value().getMask());
  }

  if (swAcl->getLookupClassL2()) {
    auto lookupClassL2Expected =
        static_cast<int>(swAcl->getLookupClassL2().value());
    auto aclFieldFdbDstUserMetaGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldFdbDstUserMeta());
    auto [fdbDstUserMetaDataGot, fdbDstUserMetaMaskGot] =
        aclFieldFdbDstUserMetaGot.getDataAndMask();
    EXPECT_EQ(fdbDstUserMetaDataGot, lookupClassL2Expected);

    /*
     * TODO(skhare)
     * CSP: CS00010615865
     * kLookupClassMask = 0xffffffff, but fdbDstUserMetaMaskGot is 0x000003ff.
     * Note, however, the following still holds true:
     * fdbDstUserMetaDataGot & fdbDstUserMetaMaskGot = lookupClassL2Expected
     * But SAI implementation should still return the correct mask 0xffffffff.
     */
    // EXPECT_EQ(fdbDstUserMetaMaskGot, SaiAclTableManager::kLookupClassMask);
    std::ignore = fdbDstUserMetaMaskGot;
  }

  if (swAcl->getLookupClass()) {
    auto lookupClassExpected =
        static_cast<int>(swAcl->getLookupClass().value());
    auto aclFieldRouteDstUserMetaGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta());
    auto [routeDstUserMetaDataGot, routeDstUserMetaMaskGot] =
        aclFieldRouteDstUserMetaGot.getDataAndMask();
    EXPECT_EQ(routeDstUserMetaDataGot, lookupClassExpected);

    /*
     * TODO(skhare)
     * CSP: CS00010615865
     * kLookupClassMask = 0xffffffff, but roueDstUserMetaMaskGot is 0x000003ff.
     * Note, however, the following still holds true:
     * routeDstUserMetaDataGot & routeDstUserMetaMaskGot = lookupClassExpected
     * But SAI implementation should still return the correct mask 0xffffffff.
     */
    // EXPECT_EQ(routeDstUserMetaMaskGot, SaiAclTableManager::kLookupClassMask);
    std::ignore = routeDstUserMetaMaskGot;

    auto aclFieldNeighborDstUserMetaGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId,
            SaiAclEntryTraits::Attributes::FieldNeighborDstUserMeta());
    auto [neighborDstUserMetaDataGot, neighborDstUserMetaMaskGot] =
        aclFieldNeighborDstUserMetaGot.getDataAndMask();
    EXPECT_EQ(neighborDstUserMetaDataGot, lookupClassExpected);

    /*
     * TODO(skhare)
     * CSP: CS00010615865
     * kLookupClassMask = 0xffffffff, but neighborDstUserMetaMaskGot is
     * 0x000003ff. Note, however, the following still holds true:
     * neighborDstUserMetaDataGot & neighborDstUserMetaMaskGot =
     * lookupClassExpected
     *
     * But SAI implementation should still return the * correct mask 0xffffffff.
     */
    // EXPECT_EQ(neighborDstUserMetaMaskGot,
    // SaiAclTableManager::kLookupClassMask);
    std::ignore = neighborDstUserMetaMaskGot;
  }

  auto action = swAcl->getAclAction();
  if (action) {
    if (action.value().getSendToQueue()) {
      auto sendToQueue = action.value().getSendToQueue().value();
      bool sendToCpu = sendToQueue.second;
      if (!sendToCpu) {
        auto expectedQueueId =
            static_cast<sai_uint8_t>(*sendToQueue.first.queueId_ref());
        auto aclActionSetTCGot =
            SaiApiTable::getInstance()->aclApi().getAttribute(
                aclEntryId, SaiAclEntryTraits::Attributes::ActionSetTC());
        auto queueIdGot = aclActionSetTCGot.getData();
        EXPECT_EQ(queueIdGot, expectedQueueId);
      }
    }

    if (action.value().getSetDscp()) {
      const int expectedDscpValue =
          *action.value().getSetDscp().value().dscpValue_ref();

      auto aclActionSetDSCPGot =
          SaiApiTable::getInstance()->aclApi().getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::ActionSetDSCP());
      auto dscpValueGot = aclActionSetDSCPGot.getData();
      EXPECT_EQ(dscpValueGot, expectedDscpValue);
    }
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
