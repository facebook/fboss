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

struct AccessPolicyRule {
  std::string name;
  cfg::AclActionType action{cfg::AclActionType::PERMIT};
  cfg::AclLookupClassPort lookupClass{
      cfg::AclLookupClassPort::CLASS_PORT_RESTRICTED};
  std::optional<int16_t> proto;
  std::optional<int32_t> l4DstPort;
  std::optional<int32_t> l4SrcPort;
  std::optional<int16_t> tcpFlagsBitMap;
  std::optional<std::string> dstIp;
  std::optional<cfg::EtherType> etherType;
  // Production names run to 46 characters, past the 31 a SAI ACL counter label
  // holds, so counters get a short generated name instead.
  std::string counterName;
};

const std::vector<AccessPolicyRule>& accessPolicyRules();

void addAccessPolicyTables(cfg::SwitchConfig& config, AccessPolicyShape shape);

void addAccessPolicyAcls(
    cfg::SwitchConfig& config,
    const std::vector<const HwAsic*>& asics,
    AccessPolicyShape shape,
    const std::set<std::string>& omitRules = {});

cfg::AclTable* findAccessPolicyAclTable(
    cfg::SwitchConfig& config,
    const std::string& name);

} // namespace facebook::fboss::utility
