// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/utils/AccessPolicyAclTestUtils.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/FbossError.h"

namespace facebook::fboss::utility {

std::string kAccessPolicyClassIdTable() {
  return "AccessPolicyClassIDTable";
}

std::string kAccessPolicyRestrictedTable() {
  return "AccessPolicyRestrictedTable";
}

std::string kAccessPolicyTableGroup() {
  return "access-policy-ingress-ACL-Table-Group";
}

std::optional<AccessPolicyShape> accessPolicyShape(
    const std::vector<const HwAsic*>& asics) {
  auto asicType = checkSameAndGetAsic(asics)->getAsicType();
  switch (asicType) {
    case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
      return AccessPolicyShape::ClassId;
    case cfg::AsicType::ASIC_TYPE_EBRO:
      return AccessPolicyShape::PortBound;
    default:
      XLOG(INFO) << "No access policy shape for asic "
                 << apache::thrift::util::enumNameSafe(asicType);
      return std::nullopt;
  }
}

cfg::AclTable* findAccessPolicyAclTable(
    cfg::SwitchConfig& config,
    const std::string& name) {
  // utility::getAclTable() only searches the first table group at a stage.
  for (auto& group : *config.aclTableGroups()) {
    for (auto& table : *group.aclTables()) {
      if (*table.name() == name) {
        return &table;
      }
    }
  }
  throw FbossError("No ACL table named ", name);
}

} // namespace facebook::fboss::utility
