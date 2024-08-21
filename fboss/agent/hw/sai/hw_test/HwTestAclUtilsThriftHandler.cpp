// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/test/utils/AclTestUtils.h"

namespace {
std::string getActualAclTableName(
    const std::optional<std::string>& aclTableName) {
  // Previously hardcoded kAclTable1 in all use cases since only supported
  // single acl table. Now support multiple tables so can accept table name. If
  // table name not provided we are still using single table, so continue using
  // kAclTable1.
  return aclTableName.has_value() ? aclTableName.value()
                                  : facebook::fboss::kAclTable1;
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
  return isAclTableEnabled(
      std::make_unique<std::string>(facebook::fboss::kAclTable1));
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
      std::make_unique<std::string>(facebook::fboss::kAclTable1));
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

} // namespace utility
} // namespace fboss
} // namespace facebook
