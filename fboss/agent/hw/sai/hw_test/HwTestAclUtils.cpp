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
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
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
      aclTableManager.getAclEntryHandle(aclTableHandle, swAcl->getPriority());
  auto aclEntryId = aclEntryHandle->aclEntry->adapterKey();

  auto aclFieldPriorityExpected =
      aclTableManager.swPriorityToSaiPriority(swAcl->getPriority());
  auto aclFieldPriorityGot = SaiApiTable::getInstance()->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::Priority());
  EXPECT_EQ(aclFieldPriorityGot, aclFieldPriorityExpected);

  if (swAcl->getSrcIp().first) {
    if (swAcl->getSrcIp().first.isV6()) {
      auto aclFieldSrcIpV6Got =
          SaiApiTable::getInstance()->aclApi().getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::FieldSrcIpV6());
      auto [srcIpV6DataGot, srcIpV6MaskGot] =
          aclFieldSrcIpV6Got.getDataAndMask();
      auto srcIpV6MaskExpected = folly::IPAddressV6(
          folly::IPAddressV6::fetchMask(swAcl->getSrcIp().second));

      EXPECT_EQ(srcIpV6DataGot, swAcl->getSrcIp().first.asV6());
      EXPECT_EQ(srcIpV6MaskGot, srcIpV6MaskExpected);
    } else if (swAcl->getSrcIp().first.isV4()) {
      auto aclFieldSrcIpV4Got =
          SaiApiTable::getInstance()->aclApi().getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::FieldSrcIpV4());
      auto [srcIpV4DataGot, srcIpV4MaskGot] =
          aclFieldSrcIpV4Got.getDataAndMask();
      auto srcIpV4MaskExpected = folly::IPAddressV4(
          folly::IPAddressV4::fetchMask(swAcl->getSrcIp().second));

      EXPECT_EQ(srcIpV4DataGot, swAcl->getSrcIp().first.asV4());
      EXPECT_EQ(srcIpV4MaskGot, srcIpV4MaskExpected);
    }
  }

  if (swAcl->getDstIp().first) {
    if (swAcl->getDstIp().first.isV6()) {
      auto aclFieldDstIpV6Got =
          SaiApiTable::getInstance()->aclApi().getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::FieldDstIpV6());
      auto [dstIpV6DataGot, dstIpV6MaskGot] =
          aclFieldDstIpV6Got.getDataAndMask();
      auto dstIpV6MaskExpected = folly::IPAddressV6(
          folly::IPAddressV6::fetchMask(swAcl->getDstIp().second));

      EXPECT_EQ(dstIpV6DataGot, swAcl->getDstIp().first.asV6());
      EXPECT_EQ(dstIpV6MaskGot, dstIpV6MaskExpected);
    } else if (swAcl->getDstIp().first.isV4()) {
      auto aclFieldDstIpV4Got =
          SaiApiTable::getInstance()->aclApi().getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::FieldDstIpV4());
      auto [dstIpV4DataGot, dstIpV4MaskGot] =
          aclFieldDstIpV4Got.getDataAndMask();
      auto dstIpV4MaskExpected = folly::IPAddressV4(
          folly::IPAddressV4::fetchMask(swAcl->getDstIp().second));

      EXPECT_EQ(dstIpV4DataGot, swAcl->getDstIp().first.asV4());
      EXPECT_EQ(dstIpV4MaskGot, dstIpV4MaskExpected);
    }
  }

  if (swAcl->getSrcPort()) {
    const auto& portManager =
        static_cast<const SaiSwitch*>(hw)->managerTable()->portManager();
    auto portHandle =
        portManager.getPortHandle(PortID(swAcl->getSrcPort().value()));
    EXPECT_TRUE(portHandle);

    auto srcPortDataExpected = portHandle->port->adapterKey();
    auto aclFieldSrcPortGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldSrcPort());
    auto srcPortDataGot = aclFieldSrcPortGot.getDataAndMask().first;
    EXPECT_EQ(srcPortDataGot, srcPortDataExpected);
  }

  if (swAcl->getDstPort()) {
    const auto& portManager =
        static_cast<const SaiSwitch*>(hw)->managerTable()->portManager();
    auto portHandle =
        portManager.getPortHandle(PortID(swAcl->getDstPort().value()));
    EXPECT_TRUE(portHandle);

    auto dstPortDataExpected = portHandle->port->adapterKey();
    auto aclFieldDstPortGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldOutPort());
    auto dstPortDataGot = aclFieldDstPortGot.getDataAndMask().first;
    EXPECT_EQ(dstPortDataGot, dstPortDataExpected);
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

  if (swAcl->getIpFrag()) {
    auto aclFieldIpFragDataExpected =
        aclTableManager.cfgIpFragToSaiIpFrag(swAcl->getIpFrag().value());
    auto aclFieldIpFragGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIpFrag());
    auto [ipFragDataGot, ipFragMaskGot] = aclFieldIpFragGot.getDataAndMask();
    EXPECT_EQ(ipFragDataGot, aclFieldIpFragDataExpected);
    EXPECT_EQ(ipFragMaskGot, SaiAclTableManager::kMaskDontCare);
  }

  if (swAcl->getIcmpType()) {
    if (swAcl->getProto()) {
      if (swAcl->getProto().value() == AclEntryFields::kProtoIcmp) {
        auto aclFieldIcmpV4TypeGot =
            SaiApiTable::getInstance()->aclApi().getAttribute(
                aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV4Type());
        auto [icmpV4TypeDataGot, icmpV4TypeMaskGot] =
            aclFieldIcmpV4TypeGot.getDataAndMask();

        EXPECT_EQ(icmpV4TypeDataGot, swAcl->getIcmpType().value());
        EXPECT_EQ(icmpV4TypeMaskGot, SaiAclTableManager::kIcmpTypeMask);

        if (swAcl->getIcmpCode()) {
          auto aclFieldIcmpV4CodeGot =
              SaiApiTable::getInstance()->aclApi().getAttribute(
                  aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV4Code());
          auto [icmpV4CodeDataGot, icmpV4CodeMaskGot] =
              aclFieldIcmpV4CodeGot.getDataAndMask();

          EXPECT_EQ(icmpV4CodeDataGot, swAcl->getIcmpCode().value());
          EXPECT_EQ(icmpV4CodeMaskGot, SaiAclTableManager::kIcmpCodeMask);
        }
      } else if (swAcl->getProto().value() == AclEntryFields::kProtoIcmpv6) {
        auto aclFieldIcmpV6TypeGot =
            SaiApiTable::getInstance()->aclApi().getAttribute(
                aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV6Type());
        auto [icmpV6TypeDataGot, icmpV6TypeMaskGot] =
            aclFieldIcmpV6TypeGot.getDataAndMask();

        EXPECT_EQ(icmpV6TypeDataGot, swAcl->getIcmpType().value());
        EXPECT_EQ(icmpV6TypeMaskGot, SaiAclTableManager::kIcmpTypeMask);

        if (swAcl->getIcmpCode()) {
          auto aclFieldIcmpV6CodeGot =
              SaiApiTable::getInstance()->aclApi().getAttribute(
                  aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV6Code());
          auto [icmpV6CodeDataGot, icmpV6CodeMaskGot] =
              aclFieldIcmpV6CodeGot.getDataAndMask();

          EXPECT_EQ(icmpV6CodeDataGot, swAcl->getIcmpCode().value());
          EXPECT_EQ(icmpV6CodeMaskGot, SaiAclTableManager::kIcmpCodeMask);
        }
      }
    }
  }

  if (swAcl->getDscp()) {
    auto aclFieldDscpGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDscp());
    auto [dscpVal, dscpMask] = aclFieldDscpGot.getDataAndMask();
    EXPECT_EQ(dscpVal, swAcl->getDscp().value());
    EXPECT_EQ(dscpMask, SaiAclTableManager::kDscpMask);
  }

  if (swAcl->getDstMac()) {
    auto aclFieldDstMacGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDstMac());
    auto [dstMacData, dstMacMask] = aclFieldDstMacGot.getDataAndMask();
    EXPECT_EQ(dstMacData, swAcl->getDstMac().value());
    EXPECT_EQ(dstMacMask, SaiAclTableManager::kMacMask());
  }

  if (swAcl->getIpType()) {
    auto aclFieldIpTypeDataExpected =
        aclTableManager.cfgIpTypeToSaiIpType(swAcl->getIpType().value());
    auto aclFieldIpTypeGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIpType());

    EXPECT_EQ(
        aclFieldIpTypeGot.getDataAndMask().first, aclFieldIpTypeDataExpected);
  }

  if (swAcl->getTtl()) {
    auto aclFieldTtlGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldTtl());
    auto [ttlVal, ttlMask] = aclFieldTtlGot.getDataAndMask();
    EXPECT_EQ(ttlVal, swAcl->getTtl().value().getValue());
    EXPECT_EQ(ttlMask, swAcl->getTtl().value().getMask());
  }

  if (swAcl->getLookupClassL2()) {
    auto fdbMetaDataAndMaskExpected =
        aclTableManager.cfgLookupClassToSaiFdbMetaDataAndMask(
            swAcl->getLookupClassL2().value());

    auto aclFieldFdbDstUserMetaGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldFdbDstUserMeta());
    EXPECT_EQ(
        aclFieldFdbDstUserMetaGot.getDataAndMask(), fdbMetaDataAndMaskExpected);
  }

  if (swAcl->getLookupClass()) {
    auto routeMetaDataAndMaskExpected =
        aclTableManager.cfgLookupClassToSaiRouteMetaDataAndMask(
            swAcl->getLookupClass().value());
    auto aclFieldRouteDstUserMetaGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta());
    EXPECT_EQ(
        aclFieldRouteDstUserMetaGot.getDataAndMask(),
        routeMetaDataAndMaskExpected);

    auto neighborMetaDataAndMaskExpected =
        aclTableManager.cfgLookupClassToSaiNeighborMetaDataAndMask(
            swAcl->getLookupClass().value());
    auto aclFieldNeighborDstUserMetaGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId,
            SaiAclEntryTraits::Attributes::FieldNeighborDstUserMeta());
    EXPECT_EQ(
        aclFieldNeighborDstUserMetaGot.getDataAndMask(),
        neighborMetaDataAndMaskExpected);
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

void checkAclEntryAndStatCount(
    const HwSwitch* hwSwitch,
    int aclCount,
    int aclStatCount,
    int counterCount) {
  const auto& aclTableManager = static_cast<const SaiSwitch*>(hwSwitch)
                                    ->managerTable()
                                    ->aclTableManager();
  auto aclTableId = aclTableManager.getAclTableHandle(SaiSwitch::kAclTable1)
                        ->aclTable->adapterKey();

  auto aclTableEntryListGot = SaiApiTable::getInstance()->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::EntryList());

  EXPECT_EQ(aclCount, aclTableEntryListGot.size());

  int aclStatCountGot = 0;
  int counterCountGot = 0;
  for (const auto& aclEntryId : aclTableEntryListGot) {
    auto aclCounterIdGot =
        SaiApiTable::getInstance()
            ->aclApi()
            .getAttribute(
                AclEntrySaiId(aclEntryId),
                SaiAclEntryTraits::Attributes::ActionCounter())
            .getData();

    if (aclCounterIdGot != SAI_NULL_OBJECT_ID) {
      aclStatCountGot++;

      auto enablePacketCount =
          SaiApiTable::getInstance()->aclApi().getAttribute(
              AclCounterSaiId(aclCounterIdGot),
              SaiAclCounterTraits::Attributes::EnablePacketCount());

      if (enablePacketCount) {
        counterCountGot++;
      }

      auto enableByteCount = SaiApiTable::getInstance()->aclApi().getAttribute(
          AclCounterSaiId(aclCounterIdGot),
          SaiAclCounterTraits::Attributes::EnableByteCount());

      if (enableByteCount) {
        counterCountGot++;
      }
    }
  }

  EXPECT_EQ(aclStatCount, aclStatCountGot);
  EXPECT_EQ(counterCount, counterCountGot);
}

void checkAclStat(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> state,
    std::vector<std::string> acls,
    const std::string& statName,
    std::vector<cfg::CounterType> counterTypes) {
  for (const auto& aclName : acls) {
    auto swAcl = state->getAcl(aclName);
    auto swTrafficCounter = getAclTrafficCounter(state, aclName);
    ASSERT_TRUE(swTrafficCounter);
    ASSERT_EQ(statName, *swTrafficCounter->name_ref());

    const auto& aclTableManager =
        static_cast<const SaiSwitch*>(hw)->managerTable()->aclTableManager();
    auto aclTableHandle =
        aclTableManager.getAclTableHandle(SaiSwitch::kAclTable1);
    auto aclEntryHandle =
        aclTableManager.getAclEntryHandle(aclTableHandle, swAcl->getPriority());
    auto aclEntryId = aclEntryHandle->aclEntry->adapterKey();

    auto aclCounterIdGot =
        SaiApiTable::getInstance()
            ->aclApi()
            .getAttribute(
                AclEntrySaiId(aclEntryId),
                SaiAclEntryTraits::Attributes::ActionCounter())
            .getData();

    bool packetCountEnabledGot = false;
    bool byteCountEnabledGot = false;

    if (aclCounterIdGot != SAI_NULL_OBJECT_ID) {
      packetCountEnabledGot = SaiApiTable::getInstance()->aclApi().getAttribute(
          AclCounterSaiId(aclCounterIdGot),
          SaiAclCounterTraits::Attributes::EnablePacketCount());
      byteCountEnabledGot = SaiApiTable::getInstance()->aclApi().getAttribute(
          AclCounterSaiId(aclCounterIdGot),
          SaiAclCounterTraits::Attributes::EnableByteCount());
    }

    for (auto counterType : counterTypes) {
      switch (counterType) {
        case cfg::CounterType::PACKETS:
          EXPECT_TRUE(packetCountEnabledGot);
          break;
        case cfg::CounterType::BYTES:
          EXPECT_TRUE(byteCountEnabledGot);
          break;
        default:
          EXPECT_FALSE(true);
      }
    }
  }
}

void checkAclStatDeleted(
    const HwSwitch* /*hwSwitch*/,
    const std::string& /*statName*/) {
  throw FbossError("Not implemented");
}

void checkAclStatSize(
    const HwSwitch* /*hwSwitch*/,
    const std::string& /*statName*/) {
  throw FbossError("Not implemented");
}

uint64_t getAclInOutPackets(
    const HwSwitch* hw,
    std::shared_ptr<SwitchState> state,
    const std::string& aclName,
    const std::string& /*statName*/) {
  auto swAcl = state->getAcl(aclName);
  const auto& aclTableManager =
      static_cast<const SaiSwitch*>(hw)->managerTable()->aclTableManager();
  auto aclTableHandle =
      aclTableManager.getAclTableHandle(SaiSwitch::kAclTable1);
  auto aclEntryHandle =
      aclTableManager.getAclEntryHandle(aclTableHandle, swAcl->getPriority());
  auto aclEntryId = aclEntryHandle->aclEntry->adapterKey();

  auto aclCounterIdGot = SaiApiTable::getInstance()
                             ->aclApi()
                             .getAttribute(
                                 AclEntrySaiId(aclEntryId),
                                 SaiAclEntryTraits::Attributes::ActionCounter())
                             .getData();

  auto counterPackets = SaiApiTable::getInstance()->aclApi().getAttribute(
      AclCounterSaiId(aclCounterIdGot),
      SaiAclCounterTraits::Attributes::CounterPackets());

  return counterPackets;
}

} // namespace facebook::fboss::utility
