// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/test/utils/AclTestUtils.h"

namespace {
std::string getActualAclTableName(
    const std::optional<std::string>& aclTableName) {
  // Previously hardcoded kAclTable1 in all use cases since only supported
  // single acl table. Now support multiple tables so can accept table name. If
  // table name not provided we are still using single table, so continue using
  // kAclTable1.
  return aclTableName.has_value()
      ? aclTableName.value()
      : facebook::fboss::cfg::switch_config_constants::
            DEFAULT_INGRESS_ACL_TABLE();
}
} // namespace

namespace facebook {
namespace fboss {
namespace utility {

int32_t HwTestThriftHandler::getAclTableNumAclEntries(
    std::unique_ptr<std::string> name) {
  const auto& aclTableManager = static_cast<const SaiSwitch*>(hwSwitch_)
                                    ->managerTable()
                                    ->aclTableManager();
  auto aclTableId =
      aclTableManager.getAclTableHandle(getActualAclTableName(*name))
          ->aclTable->adapterKey();

  auto aclTableEntryListGot = SaiApiTable::getInstance()->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::EntryList());

  return aclTableEntryListGot.size();
}

bool HwTestThriftHandler::isDefaultAclTableEnabled() {
  return isAclTableEnabled(std::make_unique<std::string>(
      cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE()));
}

bool HwTestThriftHandler::isAclTableEnabled(std::unique_ptr<std::string> name) {
  if (!name) {
    return isDefaultAclTableEnabled();
  }
  const auto& aclTableName = *name;
  const auto& aclTableManager = static_cast<const SaiSwitch*>(hwSwitch_)
                                    ->managerTable()
                                    ->aclTableManager();
  auto aclTableHandle =
      aclTableManager.getAclTableHandle(getActualAclTableName(aclTableName));

  return aclTableHandle != nullptr;
}

int32_t HwTestThriftHandler::getDefaultAclTableNumAclEntries() {
  const auto& aclTableManager = static_cast<const SaiSwitch*>(hwSwitch_)
                                    ->managerTable()
                                    ->aclTableManager();
  auto tableName = getActualAclTableName(std::nullopt);
  auto aclTableHandle = aclTableManager.getAclTableHandle(tableName);
  if (!aclTableHandle) {
    throw FbossError("ACL table", tableName, " not found");
  }
  const auto aclTableId = aclTableManager
                              .getAclTableHandle(getActualAclTableName(
                                  getActualAclTableName(std::nullopt)))
                              ->aclTable->adapterKey();

  auto aclTableEntryListGot = SaiApiTable::getInstance()->aclApi().getAttribute(
      aclTableId, SaiAclTableTraits::Attributes::EntryList());

  return aclTableEntryListGot.size();
}

bool HwTestThriftHandler::isStatProgrammedInDefaultAclTable(
    std::unique_ptr<std::vector<::std::string>> aclEntryNames,
    std::unique_ptr<std::string> counterName,
    std::unique_ptr<std::vector<cfg::CounterType>> types) {
  return isStatProgrammedInAclTable(
      std::move(aclEntryNames),
      std::move(counterName),
      std::move(types),
      std::make_unique<std::string>(
          cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE()));
}

bool HwTestThriftHandler::isStatProgrammedInAclTable(
    std::unique_ptr<std::vector<::std::string>> aclEntryNames,
    std::unique_ptr<std::string> counterName,
    std::unique_ptr<std::vector<cfg::CounterType>> types,
    std::unique_ptr<std::string> tableName) {
  auto state = hwSwitch_->getProgrammedState();

  for (const auto& aclName : *aclEntryNames) {
    auto swAcl = getAclEntryByName(state, aclName);
    auto swTrafficCounter = getAclTrafficCounter(state, aclName);
    if (!swTrafficCounter || *swTrafficCounter->name() != *counterName) {
      return false;
    }

    const auto& aclTableManager = static_cast<const SaiSwitch*>(hwSwitch_)
                                      ->managerTable()
                                      ->aclTableManager();
    auto aclTableHandle =
        aclTableManager.getAclTableHandle(getActualAclTableName(*tableName));
    auto aclEntryHandle =
        aclTableManager.getAclEntryHandle(aclTableHandle, swAcl->getPriority());
    auto aclEntryId = aclEntryHandle->aclEntry->adapterKey();

    // Get counter corresponding to the ACL entry
    auto aclCounterIdGot =
        SaiApiTable::getInstance()
            ->aclApi()
            .getAttribute(
                AclEntrySaiId(aclEntryId),
                SaiAclEntryTraits::Attributes::ActionCounter())
            .getData();
    if (aclCounterIdGot == SAI_NULL_OBJECT_ID) {
      return false;
    }

    bool packetCountEnabledExpected = false;
    bool byteCountEnabledExpected = false;

#if SAI_API_VERSION >= SAI_VERSION(1, 10, 2)
    // Counter name must match what was previously configured
    auto aclCounterNameGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        AclCounterSaiId(aclCounterIdGot),
        SaiAclCounterTraits::Attributes::Label());
    std::string aclCounterNameGotStr(aclCounterNameGot.data());
    if (*counterName != aclCounterNameGotStr) {
      return false;
    }

    // Verify that only the configured 'types' (byte/packet) of counters are
    // configured.
    for (auto counterType : *types) {
      switch (counterType) {
        case cfg::CounterType::PACKETS:
          packetCountEnabledExpected = true;
          break;
        case cfg::CounterType::BYTES:
          byteCountEnabledExpected = true;
          break;
        default:
          return false;
      }
    }

    bool packetCountEnabledGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            AclCounterSaiId(aclCounterIdGot),
            SaiAclCounterTraits::Attributes::EnablePacketCount());
    bool byteCountEnabledGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            AclCounterSaiId(aclCounterIdGot),
            SaiAclCounterTraits::Attributes::EnableByteCount());

    if (packetCountEnabledExpected != packetCountEnabledGot ||
        byteCountEnabledExpected != byteCountEnabledGot) {
      return false;
    }
#else
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

    for (auto counterType : *types) {
      switch (counterType) {
        case cfg::CounterType::PACKETS:
          if (packetCountEnabledGot != packetCountEnabledExpected) {
            return false;
          }
          break;
        case cfg::CounterType::BYTES:
          if (byteCountEnabledGot != byteCountEnabledExpected) {
            return false;
          }
          break;
        default:
          return false;
      }
    }
#endif
  }
  return true;
}

bool HwTestThriftHandler::isAclEntrySame(
    std::unique_ptr<state::AclEntryFields> aclEntry,
    std::unique_ptr<std::string> aclTableName) {
  auto state = hwSwitch_->getProgrammedState();
  auto swAcl = std::make_shared<AclEntry>(*aclEntry);

  const auto& aclTableManager = static_cast<const SaiSwitch*>(hwSwitch_)
                                    ->managerTable()
                                    ->aclTableManager();

  auto tableName = aclTableName ? getActualAclTableName(*aclTableName)
                                : getActualAclTableName(std::nullopt);
  auto aclTableHandle = aclTableManager.getAclTableHandle(tableName);
  auto aclEntryHandle =
      aclTableManager.getAclEntryHandle(aclTableHandle, swAcl->getPriority());
  auto aclEntryId = aclEntryHandle->aclEntry->adapterKey();

  auto aclFieldPriorityExpected =
      aclTableManager.swPriorityToSaiPriority(swAcl->getPriority());
  auto aclFieldPriorityGot = SaiApiTable::getInstance()->aclApi().getAttribute(
      aclEntryId, SaiAclEntryTraits::Attributes::Priority());
  if (aclFieldPriorityGot != aclFieldPriorityExpected) {
    return false;
  }

  if (swAcl->getSrcIp().first) {
    if (swAcl->getSrcIp().first.isV6()) {
      auto aclFieldSrcIpV6Got =
          SaiApiTable::getInstance()->aclApi().getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::FieldSrcIpV6());
      auto [srcIpV6DataGot, srcIpV6MaskGot] =
          aclFieldSrcIpV6Got.getDataAndMask();
      auto srcIpV6MaskExpected = folly::IPAddressV6(
          folly::IPAddressV6::fetchMask(swAcl->getSrcIp().second));

      if (srcIpV6DataGot != swAcl->getSrcIp().first.asV6() ||
          srcIpV6MaskGot != srcIpV6MaskExpected) {
        return false;
      }
    } else if (swAcl->getSrcIp().first.isV4()) {
      auto aclFieldSrcIpV4Got =
          SaiApiTable::getInstance()->aclApi().getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::FieldSrcIpV4());
      auto [srcIpV4DataGot, srcIpV4MaskGot] =
          aclFieldSrcIpV4Got.getDataAndMask();
      auto srcIpV4MaskExpected = folly::IPAddressV4(
          folly::IPAddressV4::fetchMask(swAcl->getSrcIp().second));

      if (srcIpV4DataGot != swAcl->getSrcIp().first.asV4() ||
          srcIpV4MaskGot != srcIpV4MaskExpected) {
        return false;
      }
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

      if (dstIpV6DataGot != swAcl->getDstIp().first.asV6() ||
          dstIpV6MaskGot != dstIpV6MaskExpected) {
        return false;
      }
    } else if (swAcl->getDstIp().first.isV4()) {
      auto aclFieldDstIpV4Got =
          SaiApiTable::getInstance()->aclApi().getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::FieldDstIpV4());
      auto [dstIpV4DataGot, dstIpV4MaskGot] =
          aclFieldDstIpV4Got.getDataAndMask();
      auto dstIpV4MaskExpected = folly::IPAddressV4(
          folly::IPAddressV4::fetchMask(swAcl->getDstIp().second));

      if (dstIpV4DataGot != swAcl->getDstIp().first.asV4() ||
          dstIpV4MaskGot != dstIpV4MaskExpected) {
        return false;
      }
    }
  }

  if (swAcl->getSrcPort()) {
    const auto& portManager =
        static_cast<const SaiSwitch*>(hwSwitch_)->managerTable()->portManager();
    auto portHandle =
        portManager.getPortHandle(PortID(swAcl->getSrcPort().value()));
    if (!portHandle) {
      return false;
    }

    auto srcPortDataExpected = portHandle->port->adapterKey();
    auto aclFieldSrcPortGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldSrcPort());
    auto srcPortDataGot = aclFieldSrcPortGot.getDataAndMask().first;
    if (srcPortDataGot != srcPortDataExpected) {
      return false;
    }
  }

  if (swAcl->getDstPort()) {
    const auto& portManager =
        static_cast<const SaiSwitch*>(hwSwitch_)->managerTable()->portManager();
    auto portHandle =
        portManager.getPortHandle(PortID(swAcl->getDstPort().value()));
    if (!portHandle) {
      return false;
    }

    auto dstPortDataExpected = portHandle->port->adapterKey();
    auto aclFieldDstPortGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldOutPort());
    auto dstPortDataGot = aclFieldDstPortGot.getDataAndMask().first;
    if (dstPortDataGot != dstPortDataExpected) {
      return false;
    }
  }

  if (swAcl->getL4SrcPort()) {
    auto aclFieldL4SrcPortGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldL4SrcPort());
    auto [l4SrcPortDataGot, l4SrcPortMaskGot] =
        aclFieldL4SrcPortGot.getDataAndMask();
    if (l4SrcPortDataGot != swAcl->getL4SrcPort().value() ||
        l4SrcPortMaskGot != SaiAclTableManager::kL4PortMask) {
      return false;
    }
  }

  if (swAcl->getL4DstPort()) {
    auto aclFieldL4DstPortGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldL4DstPort());
    auto [l4DstPortDataGot, l4DstPortMaskGot] =
        aclFieldL4DstPortGot.getDataAndMask();
    if (l4DstPortDataGot != swAcl->getL4DstPort().value() ||
        l4DstPortMaskGot != SaiAclTableManager::kL4PortMask) {
      return false;
    }
  }

  if (swAcl->getProto()) {
    auto aclFieldIpProtocolGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldIpProtocol());
    auto [ipProtocolDataGot, ipProtocolMaskGot] =
        aclFieldIpProtocolGot.getDataAndMask();
    if (ipProtocolDataGot != swAcl->getProto().value() ||
        ipProtocolMaskGot != SaiAclTableManager::kIpProtocolMask) {
      return false;
    }
  }

  if (swAcl->getTcpFlagsBitMap()) {
    auto aclFieldTcpFlagsGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldTcpFlags());
    auto [tcpFlagsDataGot, tcpFlagsMaskGot] =
        aclFieldTcpFlagsGot.getDataAndMask();

    if (tcpFlagsDataGot != swAcl->getTcpFlagsBitMap().value() ||
        tcpFlagsMaskGot != SaiAclTableManager::kTcpFlagsMask) {
      return false;
    }
  }

  if (swAcl->getIpFrag()) {
    auto aclFieldIpFragDataExpected =
        aclTableManager.cfgIpFragToSaiIpFrag(swAcl->getIpFrag().value());
    auto aclFieldIpFragGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIpFrag());
    auto [ipFragDataGot, ipFragMaskGot] = aclFieldIpFragGot.getDataAndMask();
    if (ipFragDataGot != aclFieldIpFragDataExpected ||
        ipFragMaskGot != SaiAclTableManager::kMaskDontCare) {
      return false;
    }
  }

  if (swAcl->getIcmpType()) {
    if (swAcl->getProto()) {
      if (swAcl->getProto().value() == AclEntry::kProtoIcmp) {
        auto aclFieldIcmpV4TypeGot =
            SaiApiTable::getInstance()->aclApi().getAttribute(
                aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV4Type());
        auto [icmpV4TypeDataGot, icmpV4TypeMaskGot] =
            aclFieldIcmpV4TypeGot.getDataAndMask();

        if (icmpV4TypeDataGot != swAcl->getIcmpType().value() ||
            icmpV4TypeMaskGot != SaiAclTableManager::kIcmpTypeMask) {
          return false;
        }

        if (swAcl->getIcmpCode()) {
          auto aclFieldIcmpV4CodeGot =
              SaiApiTable::getInstance()->aclApi().getAttribute(
                  aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV4Code());
          auto [icmpV4CodeDataGot, icmpV4CodeMaskGot] =
              aclFieldIcmpV4CodeGot.getDataAndMask();

          if (icmpV4CodeDataGot != swAcl->getIcmpCode().value() ||
              icmpV4CodeMaskGot != SaiAclTableManager::kIcmpCodeMask) {
            return false;
          }
        }
      } else if (swAcl->getProto().value() == AclEntry::kProtoIcmpv6) {
        auto aclFieldIcmpV6TypeGot =
            SaiApiTable::getInstance()->aclApi().getAttribute(
                aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV6Type());
        auto [icmpV6TypeDataGot, icmpV6TypeMaskGot] =
            aclFieldIcmpV6TypeGot.getDataAndMask();

        if (icmpV6TypeDataGot != swAcl->getIcmpType().value() ||
            icmpV6TypeMaskGot != SaiAclTableManager::kIcmpTypeMask) {
          return false;
        }

        if (swAcl->getIcmpCode()) {
          auto aclFieldIcmpV6CodeGot =
              SaiApiTable::getInstance()->aclApi().getAttribute(
                  aclEntryId, SaiAclEntryTraits::Attributes::FieldIcmpV6Code());
          auto [icmpV6CodeDataGot, icmpV6CodeMaskGot] =
              aclFieldIcmpV6CodeGot.getDataAndMask();

          if (icmpV6CodeDataGot != swAcl->getIcmpCode().value() ||
              icmpV6CodeMaskGot != SaiAclTableManager::kIcmpCodeMask) {
            return false;
          }
        }
      }
    }
  }

  if (swAcl->getDscp()) {
    auto aclFieldDscpGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDscp());
    auto [dscpVal, dscpMask] = aclFieldDscpGot.getDataAndMask();
    if (dscpVal != swAcl->getDscp().value() ||
        dscpMask != SaiAclTableManager::kDscpMask) {
      return false;
    }
  }

  if (swAcl->getDstMac()) {
    auto aclFieldDstMacGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldDstMac());
    auto [dstMacData, dstMacMask] = aclFieldDstMacGot.getDataAndMask();
    if (dstMacData != swAcl->getDstMac().value() ||
        dstMacMask != SaiAclTableManager::kMacMask()) {
      return false;
    }
  }
  if (swAcl->getVlanID()) {
    auto aclFieldVlanIdGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldOuterVlanId());
    auto [vlanVal, vlanMask] = aclFieldVlanIdGot.getDataAndMask();
    if (vlanVal != swAcl->getVlanID().value() ||
        vlanMask != SaiAclTableManager::kOuterVlanIdMask) {
      return false;
    }
  }

  if (swAcl->getIpType()) {
    auto aclFieldIpTypeDataExpected =
        aclTableManager.cfgIpTypeToSaiIpType(swAcl->getIpType().value());
    auto aclFieldIpTypeGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldIpType());

    if (aclFieldIpTypeGot.getDataAndMask().first !=
        aclFieldIpTypeDataExpected) {
      return false;
    }
  }

  if (swAcl->getTtl()) {
    auto aclFieldTtlGot = SaiApiTable::getInstance()->aclApi().getAttribute(
        aclEntryId, SaiAclEntryTraits::Attributes::FieldTtl());
    auto [ttlVal, ttlMask] = aclFieldTtlGot.getDataAndMask();
    if (ttlVal != swAcl->getTtl().value().getValue() ||
        ttlMask != swAcl->getTtl().value().getMask()) {
      return false;
    }
  }

  if (swAcl->getLookupClassL2()) {
    auto fdbMetaDataAndMaskExpected =
        aclTableManager.cfgLookupClassToSaiFdbMetaDataAndMask(
            swAcl->getLookupClassL2().value());

    auto aclFieldFdbDstUserMetaGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldFdbDstUserMeta());
    if (aclFieldFdbDstUserMetaGot.getDataAndMask() !=
        fdbMetaDataAndMaskExpected) {
      return false;
    }
  }

  if (swAcl->getLookupClassRoute()) {
    auto routeMetaDataAndMaskExpected =
        aclTableManager.cfgLookupClassToSaiRouteMetaDataAndMask(
            swAcl->getLookupClassRoute().value());

    auto aclFieldRouteDstUserMetaGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId, SaiAclEntryTraits::Attributes::FieldRouteDstUserMeta());
    if (aclFieldRouteDstUserMetaGot.getDataAndMask() !=
        routeMetaDataAndMaskExpected) {
      return false;
    }
  }

  if (swAcl->getLookupClassNeighbor()) {
    auto neighborMetaDataAndMaskExpected =
        aclTableManager.cfgLookupClassToSaiNeighborMetaDataAndMask(
            swAcl->getLookupClassNeighbor().value());
    auto aclFieldNeighborDstUserMetaGot =
        SaiApiTable::getInstance()->aclApi().getAttribute(
            aclEntryId,
            SaiAclEntryTraits::Attributes::FieldNeighborDstUserMeta());
    if (aclFieldNeighborDstUserMetaGot.getDataAndMask() !=
        neighborMetaDataAndMaskExpected) {
      return false;
    }
  }

  auto action = swAcl->getAclAction();
  if (action) {
    // THRIFT_COPY
    auto matchAction = MatchAction::fromThrift(action->toThrift());
    if (matchAction.getSetTc()) {
      auto setTc = matchAction.getSetTc().value();
      bool sendToCpu = setTc.second;
      if (!sendToCpu) {
        auto expectedTcValue = static_cast<sai_uint8_t>(*setTc.first.tcValue());
        auto aclActionSetTCGot =
            SaiApiTable::getInstance()->aclApi().getAttribute(
                aclEntryId, SaiAclEntryTraits::Attributes::ActionSetTC());
        auto tcValueGot = aclActionSetTCGot.getData();
        if (tcValueGot != expectedTcValue) {
          return false;
        }
      }
    }

    if (matchAction.getSetDscp()) {
      const int expectedDscpValue =
          *matchAction.getSetDscp().value().dscpValue();

      auto aclActionSetDSCPGot =
          SaiApiTable::getInstance()->aclApi().getAttribute(
              aclEntryId, SaiAclEntryTraits::Attributes::ActionSetDSCP());
      auto dscpValueGot = aclActionSetDSCPGot.getData();
      if (dscpValueGot != expectedDscpValue) {
        return false;
      }
    }
  }
  return true;
}

bool HwTestThriftHandler::areAllAclEntriesEnabled() {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch_);
  const auto& aclTableManager = saiSwitch->managerTable()->aclTableManager();

  auto aclTableNames = aclTableManager.getAllHandleNames();
  for (const auto& aclTableName : aclTableNames) {
    if (!isAclTableEnabled(std::make_unique<std::string>(aclTableName))) {
      return false;
    }
    auto aclTableHandle = aclTableManager.getAclTableHandle(aclTableName);
    for (const auto& [prio, aclEntryHandle] : aclTableHandle->aclTableMembers) {
      auto attributes = aclEntryHandle->aclEntry->attributes();
      auto enabled = GET_ATTR(AclEntry, Enabled, attributes);
      if (!enabled) {
        return false;
      }
    }
  }
  return true;
}

} // namespace utility
} // namespace fboss
} // namespace facebook
