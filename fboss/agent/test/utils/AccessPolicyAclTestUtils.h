// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <set>
#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

// Tomahawk3 carries the access class in Port.userMetaData and matches it with a
// LOOKUP_CLASS_PORT qualifier in a table the switch owns; Ebro has no such
// qualifier and binds the table to the restricted ports instead.
enum class AccessPolicyShape { ClassId, PortBound };

std::optional<AccessPolicyShape> accessPolicyShape(
    const std::vector<const HwAsic*>& asics);

std::string kAccessPolicyClassIdTable();
std::string kAccessPolicyRestrictedTable();
std::string kAccessPolicyTableGroup();

cfg::AclTable* findAccessPolicyAclTable(
    cfg::SwitchConfig& config,
    const std::string& name);

} // namespace facebook::fboss::utility
