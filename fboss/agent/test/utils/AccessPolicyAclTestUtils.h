// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <array>
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

// Named after the configerator sources: V0 is access_policy_rules.cinc, V1 is
// access_policy_rules_v1.cinc.
enum class AccessPolicyVersion { V0, V1 };

inline constexpr std::array<AccessPolicyVersion, 2> kAccessPolicyVersions{
    AccessPolicyVersion::V0,
    AccessPolicyVersion::V1};

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

const std::vector<AccessPolicyRule>& accessPolicyRules(
    AccessPolicyVersion version = AccessPolicyVersion::V0);

// Unset fields are filled with values no rule matches, so the probe reaches the
// rule it targets and falls through to the class default once that rule is
// gone.
struct AccessPolicyProbe {
  std::string name;
  std::optional<int16_t> proto;
  std::optional<int32_t> l4DstPort;
  std::optional<int32_t> l4SrcPort;
  std::optional<int16_t> tcpFlagsBitMap;
  std::optional<std::string> dstIp;
  // Left unset by the rule derived probes below, which are all IPv6. A caller
  // building a non IP frame sets it so an ether type rule can match.
  std::optional<cfg::EtherType> etherType;
};

const std::vector<AccessPolicyProbe>& accessPolicyProbes(
    AccessPolicyVersion version = AccessPolicyVersion::V0);

// A packet shape AclTable1 or an ASIC rx reason traps to the CPU. Some of those
// traps fire ahead of the ingress ACL stage, where a deny never sees them.
enum class ControlPlanePacket {
  ArpRequest,
  ArpReply,
  NdpNeighborSolicitation,
  NdpNeighborAdvertisement,
  NdpRouterSolicitation,
  NdpRouterAdvertisement,
  Lldp,
  LldpCustomerBridge,
  Lacp,
  DhcpV4ToServer,
  DhcpV4ToClient,
  DhcpV6ToServer,
  DhcpV6ToClient,
  BgpDstPort,
  BgpSrcPort,
  Ip2Me,
  Ip2MeNetworkControl,
  LinkLocalMcast,
  LinkLocalMcastNetworkControl,
  LinkLocalUcast,
  Ttl1,
};

struct ControlPlaneProbe {
  std::string name;
  ControlPlanePacket packet;
  // What the access policy tables can qualify on. dstIp is left for the caller
  // to fill in, since the switch's own addresses are only known at run time.
  AccessPolicyProbe policyMatch;
};

const std::vector<ControlPlaneProbe>& controlPlaneProbes();

// One rule per distinct match shape, for tests that need a warm boot cycle per
// parameter and cannot afford one per rule.
const std::vector<std::string>& accessPolicyRepresentativeRules();

// First matching rule of the class, or nullopt when none matches, which is what
// an unconstrained port sees.
std::optional<AccessPolicyRule> accessPolicyMatch(
    const AccessPolicyProbe& probe,
    cfg::AclLookupClassPort lookupClass,
    const std::set<std::string>& omitRules = {},
    AccessPolicyVersion version = AccessPolicyVersion::V0);

void addAccessPolicyTables(cfg::SwitchConfig& config, AccessPolicyShape shape);

// denyAction replaces the action on every rule the rule set denies, leaving the
// permits alone.
void addAccessPolicyAcls(
    cfg::SwitchConfig& config,
    const std::vector<const HwAsic*>& asics,
    AccessPolicyShape shape,
    const std::set<std::string>& omitRules,
    cfg::AclActionType denyAction,
    AccessPolicyVersion version = AccessPolicyVersion::V0);

// Drops the tables and the stats of every version, so the caller need not know
// which one is programmed.
void removeAccessPolicy(cfg::SwitchConfig& config, AccessPolicyShape shape);

void bindAccessPolicyPort(
    cfg::SwitchConfig& config,
    AccessPolicyShape shape,
    PortID portId,
    cfg::AclLookupClassPort lookupClass);

cfg::AclTable* findAccessPolicyAclTable(
    cfg::SwitchConfig& config,
    const std::string& name);

} // namespace facebook::fboss::utility
