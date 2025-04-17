#pragma once
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"

namespace facebook::fboss::utility {

enum class AclWidth : uint8_t {
  SINGLE_WIDE = 1,
  DOUBLE_WIDE,
  TRIPLE_WIDE,
};

std::vector<cfg::AclTableQualifier> setAclQualifiers(AclWidth width);

uint32_t getMaxAclEntries(const std::vector<const HwAsic*>& asics);

void updateAclEntryFields(cfg::AclEntry* aclEntry, AclWidth width);

void addAclEntries(
    const int maxEntries,
    AclWidth width,
    cfg::SwitchConfig* cfg,
    const std::string& tableName);

} // namespace facebook::fboss::utility
