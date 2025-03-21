/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"

#include <memory>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss::utility {

std::string kDefaultAclTable() {
  return cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE();
}

std::string kTtldAclTable() {
  return "ttld-acl-table";
}

std::vector<cfg::AclTableQualifier> genAclQualifiersConfig(
    cfg::AsicType asicType) {
  std::vector<cfg::AclTableQualifier> qualifiers = {
      cfg::AclTableQualifier::SRC_IPV6,
      cfg::AclTableQualifier::DST_IPV6,
      cfg::AclTableQualifier::SRC_IPV4,
      cfg::AclTableQualifier::DST_IPV4,
      cfg::AclTableQualifier::L4_SRC_PORT,
      cfg::AclTableQualifier::L4_DST_PORT,
      cfg::AclTableQualifier::IP_PROTOCOL_NUMBER,
      cfg::AclTableQualifier::DSCP,
      cfg::AclTableQualifier::TTL,
      cfg::AclTableQualifier::ICMPV4_TYPE,
      cfg::AclTableQualifier::ICMPV4_CODE,
      cfg::AclTableQualifier::ICMPV6_TYPE,
      cfg::AclTableQualifier::ICMPV6_CODE,
      cfg::AclTableQualifier::OUTER_VLAN};
  if (asicType != cfg::AsicType::ASIC_TYPE_JERICHO3) {
    qualifiers.push_back(cfg::AclTableQualifier::IP_TYPE);
  }
  return qualifiers;
}

int getAclTableIndex(
    cfg::AclTableGroup* aclTableGroup,
    const std::string& tableName) {
  int tableIndex;
  std::vector<cfg::AclTable> aclTables = *aclTableGroup->aclTables();
  std::vector<cfg::AclTable>::iterator it = std::find_if(
      aclTables.begin(), aclTables.end(), [&](cfg::AclTable const& aclTable) {
        return *aclTable.name() == tableName;
      });
  if (it != aclTables.end()) {
    tableIndex = std::distance(aclTables.begin(), it);
  } else {
    throw FbossError(
        "Table with name ", tableName, " does not exist in config");
  }
  return tableIndex;
}

cfg::AclEntry* addAclEntry(
    cfg::SwitchConfig* cfg,
    const cfg::AclEntry& acl,
    const std::string& aclTableName,
    cfg::AclStage aclStage) {
  if (FLAGS_enable_acl_table_group) {
    auto aclTableGroup = getAclTableGroup(*cfg, aclStage);

    int tableNumber = getAclTableIndex(aclTableGroup, aclTableName);
    CHECK(aclTableGroup);
    auto& aclTable = aclTableGroup->aclTables()[tableNumber];
    if (!aclEntrySupported(&aclTable, acl)) {
      throw FbossError(
          "acl entry ",
          *acl.name(),
          " is can not be added in acl table ",
          aclTableName);
    }
    aclTable.aclEntries()->push_back(acl);
    return &aclTable.aclEntries()->back();
  } else {
    cfg->acls()->push_back(acl);
    return &cfg->acls()->back();
  }
}

cfg::AclEntry* addAcl_DEPRECATED(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const cfg::AclActionType& aclActionType,
    const std::optional<std::string>& tableName) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  *acl.actionType() = aclActionType;

  if (!tableName) {
    return addAclEntry(cfg, acl, kDefaultAclTable());
  }
  return addAclEntry(cfg, acl, *tableName);
}

cfg::AclEntry* addAcl(
    cfg::SwitchConfig* cfg,
    const cfg::AclEntry& acl,
    cfg::AclStage aclStage) {
  if (!FLAGS_enable_acl_table_group) {
    if (aclStage != cfg::AclStage::INGRESS) {
      throw FbossError(
          "Only Ingress ACL Stage is supported with single ACL table");
    }
    cfg->acls()->push_back(acl);
    return &cfg->acls()->back();
  }
  auto selectedTableName = getAclTableForAclEntry(*cfg, acl, aclStage);
  return addAclEntry(cfg, acl, selectedTableName, aclStage);
}

void addEtherTypeToAcl(
    const HwAsic* asic,
    cfg::AclEntry* acl,
    const cfg::EtherType& etherType) {
  if (asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
    acl->etherType() = etherType;
  }
}

std::shared_ptr<AclEntry> getAclEntryByName(
    const std::shared_ptr<SwitchState> state,
    const std::string& aclName) {
  std::shared_ptr<AclEntry> swAcl;
  if (FLAGS_enable_acl_table_group) {
    auto aclMap = state->getAclsForTable(
        cfg::AclStage::INGRESS,
        cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE());
    if (aclMap) {
      swAcl = aclMap->getNodeIf(aclName);
    }
  } else {
    swAcl = state->getAcl(aclName);
  }

  return swAcl;
}

std::optional<cfg::TrafficCounter> getAclTrafficCounter(
    const std::shared_ptr<SwitchState> state,
    const std::string& aclName) {
  auto swAcl = getAclEntryByName(state, aclName);
  if (swAcl && swAcl->getAclAction()) {
    return swAcl->getAclAction()
        ->cref<switch_state_tags::trafficCounter>()
        ->toThrift();
  }
  return std::nullopt;
}

std::string kDefaultAclTableGroupName() {
  return "acl-table-group-ingress";
}

std::vector<cfg::AclEntry>& getAcls(
    cfg::SwitchConfig* cfg,
    const std::optional<std::string>& tableName) {
  auto aclTableName = tableName.has_value()
      ? tableName.value()
      : cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE();
  if (FLAGS_enable_acl_table_group) {
    auto* aclTableGroup = getAclTableGroup(*cfg);
    CHECK(aclTableGroup);
    return *aclTableGroup
                ->aclTables()[getAclTableIndex(aclTableGroup, aclTableName)]
                .aclEntries();
  }
  return *cfg->acls();
}

void delAcl(
    cfg::SwitchConfig* cfg,
    const std::string& aclName,
    const std::optional<std::string>& tableName) {
  auto& acls = getAcls(cfg, tableName);

  acls.erase(
      std::remove_if(
          acls.begin(),
          acls.end(),
          [&](cfg::AclEntry const& acl) { return *acl.name() == aclName; }),
      acls.end());
}

void delLastAddedAcl(cfg::SwitchConfig* cfg) {
  auto& acls = getAcls(cfg, std::nullopt);
  acls.pop_back();
}

void addAclTableGroup(
    cfg::SwitchConfig* cfg,
    cfg::AclStage aclStage,
    const std::string& aclTableGroupName) {
  cfg::AclTableGroup cfgTableGroup;
  cfgTableGroup.name() = aclTableGroupName;
  cfgTableGroup.stage() = aclStage;
  if (auto aclTableGroup = getAclTableGroup(*cfg, aclStage)) {
    *aclTableGroup = cfgTableGroup;
    return;
  }
  if (!cfg->aclTableGroups()) {
    cfg->aclTableGroups() = {};
  }
  cfg->aclTableGroups()->push_back(std::move(cfgTableGroup));
}

void addDefaultAclTable(cfg::SwitchConfig& cfg) {
  std::optional<cfg::SdkVersion> version{};
  if (cfg.sdkVersion()) {
    version = *cfg.sdkVersion();
  }

  HwAsicTable asicTable(
      *cfg.switchSettings()->switchIdToSwitchInfo(), std::move(version));
  // TODO (pshaikh): create a method to return AclTables for a given asic type
  // and acl stage and retire this check
  auto asic = utility::checkSameAndGetAsic(asicTable.getL3Asics());
  auto split = asic->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB;

  /* Create default ACL table similar to whats being done in Agent today */
  std::vector<cfg::AclTableQualifier> qualifiers = {};
  std::vector<cfg::AclTableActionType> actions = {};
  if (!split) {
    addAclTable(
        &cfg,
        cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
        0 /* priority */,
        actions,
        qualifiers);

  } else {
    /* full set of supported and required qualifiers do not fit in single table.
     * default acl table support all use cases except TTLD and ARS */
    addAclTable(
        &cfg,
        cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE(),
        0 /* priority */,
        actions,
        {
            cfg::AclTableQualifier::DST_IPV6,
            cfg::AclTableQualifier::DST_IPV4,
            cfg::AclTableQualifier::L4_SRC_PORT,
            cfg::AclTableQualifier::L4_DST_PORT,
            cfg::AclTableQualifier::IP_PROTOCOL_NUMBER,
            cfg::AclTableQualifier::IPV6_NEXT_HEADER,
            cfg::AclTableQualifier::SRC_PORT,
            cfg::AclTableQualifier::DSCP,
            cfg::AclTableQualifier::TTL,
            cfg::AclTableQualifier::IP_TYPE,
            cfg::AclTableQualifier::ETHER_TYPE,
            cfg::AclTableQualifier::OUTER_VLAN,
        });
    addTtldAclTable(&cfg, cfg::AclStage::INGRESS, 1 /* priority */);
  }
}

cfg::AclTable* addAclTable(
    cfg::SwitchConfig* cfg,
    const std::string& aclTableName,
    const int aclTablePriority,
    const std::vector<cfg::AclTableActionType>& actionTypes,
    const std::vector<cfg::AclTableQualifier>& qualifiers,
    const std::vector<std::string>& udfGroups) {
  return addAclTable(
      cfg,
      cfg::AclStage::INGRESS,
      aclTableName,
      aclTablePriority,
      actionTypes,
      qualifiers,
      udfGroups);
}

cfg::AclTable* addAclTable(
    cfg::SwitchConfig* cfg,
    cfg::AclStage aclStage,
    const std::string& aclTableName,
    const int aclTablePriority,
    const std::vector<cfg::AclTableActionType>& actionTypes,
    const std::vector<cfg::AclTableQualifier>& qualifiers,
    const std::vector<std::string>& udfGroups) {
  auto aclTableGroup = getAclTableGroup(*cfg, aclStage);
  if (!aclTableGroup) {
    throw FbossError(
        "Attempted to add acl table ",
        aclTableName,
        " without first creating acl table group");
  }

  cfg::AclTable aclTable;
  aclTable.name() = aclTableName;
  aclTable.priority() = aclTablePriority;
  aclTable.actionTypes() = actionTypes;
  aclTable.qualifiers() = qualifiers;
  aclTable.udfGroups() = udfGroups;

  aclTableGroup->aclTables()->push_back(aclTable);
  return &aclTableGroup->aclTables()->back();
}

void delAclTable(cfg::SwitchConfig* cfg, const std::string& aclTableName) {
  auto aclTableGroup = getAclTableGroup(*cfg);
  if (!aclTableGroup) {
    throw FbossError(
        "Attempted to delete acl table (",
        aclTableName,
        ") from uninstantiated table group");
  }
  auto& aclTables = *aclTableGroup->aclTables();
  aclTables.erase(
      std::remove_if(
          aclTables.begin(),
          aclTables.end(),
          [&](cfg::AclTable const& aclTable) {
            return *aclTable.name() == aclTableName;
          }),
      aclTables.end());
}

void addAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& counterName,
    std::vector<cfg::CounterType> counterTypes) {
  auto counter = cfg::TrafficCounter();
  *counter.name() = counterName;
  if (!counterTypes.empty()) {
    *counter.types() = counterTypes;
  }
  bool counterExists = false;
  for (auto& c : *cfg->trafficCounters()) {
    if (*c.name() == counterName) {
      counterExists = true;
      break;
    }
  }
  if (!counterExists) {
    cfg->trafficCounters()->push_back(counter);
  }

  auto matchAction = cfg::MatchAction();
  matchAction.counter() = counterName;
  auto action = cfg::MatchToAction();
  *action.matcher() = matcher;
  *action.action() = matchAction;

  if (!cfg->dataPlaneTrafficPolicy()) {
    cfg->dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
  }
  cfg->dataPlaneTrafficPolicy()->matchToAction()->push_back(action);
}

void delAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& counterName) {
  int refCnt = 0;
  if (auto dataPlaneTrafficPolicy = cfg->dataPlaneTrafficPolicy()) {
    auto& matchActions = *dataPlaneTrafficPolicy->matchToAction();
    matchActions.erase(
        std::remove_if(
            matchActions.begin(),
            matchActions.end(),
            [&](cfg::MatchToAction const& matchAction) {
              if (*matchAction.matcher() == matcher &&
                  matchAction.action()->counter().value_or({}) == counterName) {
                ++refCnt;
                return true;
              }
              return false;
            }),
        matchActions.end());
  }
  if (refCnt == 0) {
    auto& counters = *cfg->trafficCounters();
    counters.erase(
        std::remove_if(
            counters.begin(),
            counters.end(),
            [&](cfg::TrafficCounter const& counter) {
              return *counter.name() == counterName;
            }),
        counters.end());
  }
}

void renameAclStat(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::string& oldCounterName,
    const std::string& newCounterName) {
  delAclStat(cfg, matcher, oldCounterName);
  addAclStat(cfg, matcher, newCounterName);
}

// Just mirror and counter for now. More can go here if needed
void addAclMatchActions(
    cfg::SwitchConfig* cfg,
    const std::string& matcher,
    const std::optional<std::string>& counterName,
    const std::optional<std::string>& mirrorName,
    bool ingress) {
  cfg::MatchAction matchAction = cfg::MatchAction();
  if (mirrorName.has_value()) {
    if (ingress) {
      matchAction.ingressMirror() = mirrorName.value();
    } else {
      matchAction.egressMirror() = mirrorName.value();
    }
  }
  if (counterName.has_value()) {
    matchAction.counter() = counterName.value();
  }
  auto matchToAction = cfg::MatchToAction();
  *matchToAction.matcher() = matcher;
  *matchToAction.action() = matchAction;

  if (!cfg->dataPlaneTrafficPolicy()) {
    cfg->dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
  }
  cfg->dataPlaneTrafficPolicy()->matchToAction()->push_back(matchToAction);
}

std::vector<cfg::CounterType> getAclCounterTypes(
    const std::vector<const HwAsic*>& asics) {
  auto asic = checkSameAndGetAsic(asics);
  // At times, it is non-trivial for SAI implementations to support enabling
  // bytes counters only or packet counters only. In such cases, SAI
  // implementations enable bytes as well as packet counters even if only
  // one of the two is enabled. FBOSS use case does not require enabling
  // only one, but always enables both packets and bytes counters. Thus,
  // enable both in the test. Reference: CS00012271364
  if (asic->isSupported(
          HwAsic::Feature::SEPARATE_BYTE_AND_PACKET_ACL_COUNTER)) {
    return {cfg::CounterType::PACKETS};
  } else {
    return {cfg::CounterType::BYTES, cfg::CounterType::PACKETS};
  }
}

uint64_t getAclInOutPackets(
    const SwSwitch* sw,
    const std::string& statName,
    bool bytes) {
  auto statStr = bytes ? statName + ".bytes" : statName + ".packets";
  auto hwSwitchStatsMap = sw->getHwSwitchStatsExpensive();
  int64_t statValue = 0;
  for (const auto& [switchIndex, hwswitchStats] : hwSwitchStatsMap) {
    auto aclStats = hwswitchStats.aclStats();
    auto entry = aclStats->statNameToCounterMap()->find(statStr);
    if (entry != aclStats->statNameToCounterMap()->end()) {
      statValue += entry->second;
    }
  }
  return statValue;
}

uint64_t getAclInOutPackets(
    const HwSwitch* hw,
    const std::string& statName,
    bool bytes) {
  auto statStr = bytes ? statName + ".bytes" : statName + ".packets";
  int64_t statValue = 0;
  const_cast<HwSwitch*>(hw)->updateStats();
  auto aclStats = hw->getAclStats();
  auto entry = aclStats.statNameToCounterMap()->find(statStr);
  if (entry != aclStats.statNameToCounterMap()->end()) {
    statValue += entry->second;
  }
  return statValue;
}

std::shared_ptr<AclEntry> getAclEntry(
    const std::shared_ptr<SwitchState>& state,
    const std::string& name,
    bool enableAclTableGroup) {
  if (enableAclTableGroup) {
    return state
        ->getAclsForTable(
            cfg::AclStage::INGRESS,
            cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE())
        ->getNodeIf(name);
  }
  return state->getAcl(name);
}

cfg::AclTableGroup* FOLLY_NULLABLE getAclTableGroup(cfg::SwitchConfig& config) {
  return getAclTableGroup(config, cfg::AclStage::INGRESS);
}

cfg::AclTableGroup* FOLLY_NULLABLE
getAclTableGroup(cfg::SwitchConfig& config, cfg::AclStage aclStage) {
  if (!config.aclTableGroups()) {
    return nullptr;
  }
  for (auto iter = config.aclTableGroups()->begin();
       iter != config.aclTableGroups()->end();
       iter++) {
    if (iter->stage().value() == aclStage) {
      return std::addressof(*iter);
    }
  }
  return nullptr;
}

void setupDefaultPostLookupIngressAclTableGroup(cfg::SwitchConfig& config) {
  if (config.switchSettings()->switchIdToSwitchInfo()->empty()) {
    return;
  }
  auto switchId2SwitchInfo = *config.switchSettings()->switchIdToSwitchInfo();
  std::optional<cfg::SdkVersion> version{};
  if (config.sdkVersion()) {
    version = *config.sdkVersion();
  }

  HwAsicTable asicTable(switchId2SwitchInfo, version);
  if (!asicTable.isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::INGRESS_POST_LOOKUP_ACL_TABLE)) {
    return;
  }

  if (getAclTableGroup(config, cfg::AclStage::INGRESS_POST_LOOKUP)) {
    return;
  }
  utility::addAclTableGroup(
      &config,
      cfg::AclStage::INGRESS_POST_LOOKUP,
      cfg::switch_config_constants::
          DEFAULT_POST_LOOKUP_INGRESS_ACL_TABLE_GROUP());
  utility::addAclTable(
      &config,
      cfg::AclStage::INGRESS_POST_LOOKUP,
      cfg::switch_config_constants::DEFAULT_POST_LOOKUP_INGRESS_ACL_TABLE(),
      0,
      {
          cfg::AclTableActionType::COUNTER,
          cfg::AclTableActionType::PACKET_ACTION,
          cfg::AclTableActionType::SET_USER_DEFINED_TRAP,
          cfg::AclTableActionType::SET_DSCP,
      },
      {
          cfg::AclTableQualifier::DSCP,
      },
      {});
}

void setupDefaultIngressAclTableGroup(cfg::SwitchConfig& config) {
  if (getAclTableGroup(config, cfg::AclStage::INGRESS)) {
    // default table group already exists
    return;
  }
  utility::addAclTableGroup(
      &config, cfg::AclStage::INGRESS, utility::kDefaultAclTableGroupName());
  utility::addDefaultAclTable(config);
}

void setupDefaultAclTableGroups(cfg::SwitchConfig& config) {
  setupDefaultIngressAclTableGroup(config);
  setupDefaultPostLookupIngressAclTableGroup(config);
}

cfg::AclTable* getAclTable(
    cfg::SwitchConfig& cfg,
    cfg::AclStage aclStage,
    const std::string& aclTableName) {
  auto aclTableGroup = getAclTableGroup(cfg, aclStage);
  if (!aclTableGroup) {
    return nullptr;
  }
  for (auto& aclTable : *aclTableGroup->aclTables()) {
    if (*aclTable.name() == aclTableName) {
      return &aclTable;
    }
  }
  return nullptr;
}

cfg::AclTable* addTtldAclTable(
    cfg::SwitchConfig* cfg,
    cfg::AclStage aclStage,
    const int aclTablePriority) {
  return addAclTable(
      cfg,
      aclStage,
      kTtldAclTable(),
      aclTablePriority,
      {
          // action types
      },
      {
          cfg::AclTableQualifier::ETHER_TYPE,
          cfg::AclTableQualifier::SRC_IPV4,
          cfg::AclTableQualifier::SRC_IPV6,
          cfg::AclTableQualifier::IP_PROTOCOL_NUMBER,
          cfg::AclTableQualifier::IP_TYPE,
          cfg::AclTableQualifier::TTL,
      },
      {
          // udf groups
      });
}

std::set<cfg::AclTableQualifier> getRequiredQualifers(
    const cfg::AclEntry& aclEntry) {
  int minValue = static_cast<int>(
      apache::thrift::TEnumTraits<cfg::AclTableQualifier>::min());
  int maxValue = static_cast<int>(
      apache::thrift::TEnumTraits<cfg::AclTableQualifier>::max());
  std::set<cfg::AclTableQualifier> requiredQualifiers{};

  auto addQualier = [&requiredQualifiers](
                        bool isSet, cfg::AclTableQualifier qualifer) {
    if (isSet) {
      requiredQualifiers.insert(qualifer);
    }
  };

  for (int iter = minValue; iter != maxValue; ++iter) {
    auto qualifier = static_cast<cfg::AclTableQualifier>(iter);
    switch (static_cast<cfg::AclTableQualifier>(qualifier)) {
      case cfg::AclTableQualifier::DST_IPV4:
        addQualier(
            aclEntry.dstIp().has_value() &&
                folly::IPAddress::createNetwork(*aclEntry.dstIp()).first.isV4(),
            qualifier);
        break;

      case cfg::AclTableQualifier::DST_IPV6:
        addQualier(
            aclEntry.dstIp().has_value() &&
                folly::IPAddress::createNetwork(*aclEntry.dstIp()).first.isV6(),
            qualifier);
        break;

      case cfg::AclTableQualifier::SRC_IPV4:
        addQualier(
            aclEntry.srcIp().has_value() &&
                folly::IPAddress::createNetwork(*aclEntry.srcIp()).first.isV4(),
            qualifier);
        break;

      case cfg::AclTableQualifier::SRC_IPV6:
        addQualier(
            aclEntry.srcIp().has_value() &&
                folly::IPAddress::createNetwork(*aclEntry.srcIp()).first.isV6(),
            qualifier);
        break;

      case cfg::AclTableQualifier::L4_SRC_PORT:
        addQualier(aclEntry.l4SrcPort().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::L4_DST_PORT:
        addQualier(aclEntry.l4DstPort().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::IP_PROTOCOL_NUMBER:
        if (aclEntry.etherType().has_value() &&
            *aclEntry.etherType() == cfg::EtherType::IPv4) {
          addQualier(aclEntry.proto().has_value(), qualifier);
        } else {
          addQualier(aclEntry.proto().has_value(), qualifier);
        }
        break;

      case cfg::AclTableQualifier::TCP_FLAGS:
        addQualier(aclEntry.tcpFlagsBitMap().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::SRC_PORT:
        addQualier(aclEntry.srcPort().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::OUT_PORT:
        addQualier(aclEntry.dstPort().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::IP_FRAG:
        addQualier(aclEntry.ipFrag().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::ICMPV4_TYPE:
        if (aclEntry.proto().has_value() && *aclEntry.proto() == 1) {
          addQualier(aclEntry.icmpType().has_value(), qualifier);
        }
        break;

      case cfg::AclTableQualifier::ICMPV4_CODE:
        if (aclEntry.proto().has_value() && *aclEntry.proto() == 1) {
          addQualier(aclEntry.icmpCode().has_value(), qualifier);
        }
        break;

      case cfg::AclTableQualifier::ICMPV6_TYPE:
        if (aclEntry.proto().has_value() && *aclEntry.proto() == 58) {
          addQualier(aclEntry.icmpType().has_value(), qualifier);
        }
        break;

      case cfg::AclTableQualifier::ICMPV6_CODE:
        if (aclEntry.proto().has_value() && *aclEntry.proto() == 58) {
          addQualier(aclEntry.icmpCode().has_value(), qualifier);
        }
        break;

      case cfg::AclTableQualifier::DSCP:
        addQualier(aclEntry.dscp().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::DST_MAC:
        addQualier(aclEntry.dstMac().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::IP_TYPE:
        addQualier(aclEntry.ipType().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::TTL:
        addQualier(aclEntry.ttl().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::LOOKUP_CLASS_L2:
        addQualier(aclEntry.lookupClassL2().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR:
        addQualier(aclEntry.lookupClassNeighbor().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE:
        addQualier(aclEntry.lookupClassRoute().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::ETHER_TYPE:
        addQualier(aclEntry.etherType().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::OUTER_VLAN:
        addQualier(aclEntry.vlanID().has_value(), qualifier);
        break;

      case cfg::AclTableQualifier::IPV6_NEXT_HEADER:
        if (aclEntry.etherType().has_value() &&
            *aclEntry.etherType() == cfg::EtherType::IPv6) {
          addQualier(aclEntry.proto().has_value(), qualifier);
        } else {
          addQualier(aclEntry.proto().has_value(), qualifier);
        }
        break;

      case cfg::AclTableQualifier::UDF:
      case cfg::AclTableQualifier::BTH_OPCODE:
        // TODO: identify when these qualifiers are required
        break;
    }
  }
  return requiredQualifiers;
}

bool aclEntrySupported(
    const cfg::AclTable* aclTable,
    const cfg::AclEntry& aclEntry) {
  auto aclTableQualifiers = aclTable->qualifiers();

  if (aclTableQualifiers->empty()) {
    // acl table supports all supported qualifiers
    return true;
  }
  auto requiredQualifiers = getRequiredQualifers(aclEntry);
  std::set<cfg::AclTableQualifier> aclTableQualifiersSet(
      aclTableQualifiers->begin(), aclTableQualifiers->end());
  std::set<cfg::AclTableQualifier> difference{};
  std::set_difference(
      requiredQualifiers.begin(),
      requiredQualifiers.end(),
      aclTableQualifiersSet.begin(),
      aclTableQualifiersSet.end(),
      std::inserter(difference, difference.begin()));

  return difference.empty();
}

std::string getAclTableForAclEntry(
    cfg::SwitchConfig& config,
    const cfg::AclEntry& aclEntry,
    cfg::AclStage stage) {
  auto aclTableGroup = getAclTableGroup(config, stage);
  if (!aclTableGroup) {
    throw FbossError("Acl table group not found");
  }
  for (auto& aclTable : *aclTableGroup->aclTables()) {
    if (aclEntrySupported(&aclTable, aclEntry)) {
      return *aclTable.name();
    }
  }
  throw FbossError("No acl table found for acl entry ", *aclEntry.name());
}

} // namespace facebook::fboss::utility
