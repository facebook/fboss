/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace facebook::fboss {

enum class AclWidth : uint8_t {
  SINGLE_WIDE = 1,
  DOUBLE_WIDE,
  TRIPLE_WIDE,
};

template <bool enableMultiAclTable>
struct EnableMultiAclTable {
  static constexpr auto multiAclTableEnabled = enableMultiAclTable;
};

using TestTypes =
    ::testing::Types<EnableMultiAclTable<false>, EnableMultiAclTable<true>>;

template <typename EnableMultiAclTableT>
class AgentAclScaleTest : public AgentHwTest {
  static auto constexpr isMultiAclEnabled =
      EnableMultiAclTableT::multiAclTableEnabled;

 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if constexpr (std::is_same_v<
                      EnableMultiAclTableT,
                      EnableMultiAclTable<false>>) {
      return {
          production_features::ProductionFeature::ACL_COUNTER,
          production_features::ProductionFeature::SINGLE_ACL_TABLE};
    } else {
      return {
          production_features::ProductionFeature::ACL_COUNTER,
          production_features::ProductionFeature::MULTI_ACL_TABLE};
    }
  }

 protected:
  void SetUp() override {
    FLAGS_enable_acl_table_group = isMultiAclEnabled;
    AgentHwTest::SetUp();
  }
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    if (isMultiAclEnabled) {
      utility::addAclTableGroup(
          &cfg, cfg::AclStage::INGRESS, utility::getAclTableGroupName());
    }
    return cfg;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_acl_table_group = true;
  }
  std::vector<cfg::AclTableQualifier> setAclQualifiers(AclWidth width) const {
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

  uint32_t getMaxSingleWideAclTables(const std::vector<const HwAsic*>& asics) {
    auto asic = utility::checkSameAndGetAsic(asics);
    auto maxAclTables = asic->getMaxAclTables();
    CHECK(maxAclTables.has_value());
    return maxAclTables.value();
  }

  uint32_t getMaxAclEntries(const std::vector<const HwAsic*>& asics) {
    auto asic = utility::checkSameAndGetAsic(asics);
    auto maxAclEntries = asic->getMaxAclEntries();
    CHECK(maxAclEntries.has_value());
    return maxAclEntries.value();
  }
};

} // namespace facebook::fboss
