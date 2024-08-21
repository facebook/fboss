// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

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

} // namespace utility
} // namespace fboss
} // namespace facebook
