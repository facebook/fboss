#include "fboss/agent/test/utils/AclScaleTestUtils.h"
#include "fboss/agent/AsicUtils.h"

namespace facebook::fboss::utility {

std::vector<cfg::AclTableQualifier> setAclQualifiers(AclWidth width) {
  std::vector<cfg::AclTableQualifier> qualifiers;

  if (width == AclWidth::SINGLE_WIDE) {
    qualifiers.push_back(cfg::AclTableQualifier::DSCP); // 8 bits
  } else if (width == AclWidth::DOUBLE_WIDE) {
    qualifiers.push_back(cfg::AclTableQualifier::SRC_IPV6); // 128 bits
    qualifiers.push_back(cfg::AclTableQualifier::DST_IPV6); // 128 bits
  } else if (width == AclWidth::TRIPLE_WIDE) {
    qualifiers.push_back(cfg::AclTableQualifier::SRC_IPV6); // 128 bits
    qualifiers.push_back(cfg::AclTableQualifier::DST_IPV6); // 128 bits
    qualifiers.push_back(cfg::AclTableQualifier::IP_TYPE); // 32 bits
    qualifiers.push_back(cfg::AclTableQualifier::L4_SRC_PORT); // 16 bits
    qualifiers.push_back(cfg::AclTableQualifier::L4_DST_PORT); // 16 bits
    qualifiers.push_back(cfg::AclTableQualifier::LOOKUP_CLASS_L2); // 16 bits
    qualifiers.push_back(cfg::AclTableQualifier::OUTER_VLAN); // 12 bits
    qualifiers.push_back(cfg::AclTableQualifier::DSCP); // 8 bits
  }
  return qualifiers;
}

uint32_t getMaxAclEntries(const std::vector<const HwAsic*>& asics) {
  auto asic = checkSameAndGetAsic(asics);
  auto maxAclEntries = asic->getMaxAclEntries();
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK4 ||
      asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK5) {
    return 16;
  }
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK3 ||
      asic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK ||
      asic->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
    CHECK(maxAclEntries.has_value());
    return maxAclEntries.value();
  }
  if (asic->getAsicType() == cfg::AsicType::ASIC_TYPE_EBRO ||
      asic->getAsicType() == cfg::AsicType::ASIC_TYPE_YUBA) {
    // Todo: check with Vendor to get the correct value
    return 64;
  }
  throw FbossError("max acl not supported");
}

void updateAclEntryFields(cfg::AclEntry* aclEntry, AclWidth width) {
  if (width == AclWidth::SINGLE_WIDE) {
    aclEntry->dscp() = 0x24;
  } else if (width == AclWidth::DOUBLE_WIDE) {
    aclEntry->srcIp() = "2401:db00::";
    aclEntry->dstIp() = "2401:db00::";
  } else if (width == AclWidth::TRIPLE_WIDE) {
    aclEntry->srcIp() = "2401:db00::";
    aclEntry->dstIp() = "2401:db00::";
    aclEntry->ipType() = cfg::IpType::IP6;
    aclEntry->l4SrcPort() = 8000;
    aclEntry->l4DstPort() = 8002;
    aclEntry->dscp() = 0x24;
  }
}

void addAclEntries(
    const int maxEntries,
    AclWidth width,
    cfg::SwitchConfig* cfg,
    const std::string& tableName) {
  for (auto i = 0; i < maxEntries; i++) {
    std::string aclEntryName = "Entry" + std::to_string(i);
    cfg::AclEntry aclEntry;
    aclEntry.name() = aclEntryName;
    aclEntry.actionType() = cfg::AclActionType::DENY;
    updateAclEntryFields(&aclEntry, width);

    utility::addAclEntry(cfg, aclEntry, tableName);
  }
}
} // namespace facebook::fboss::utility
