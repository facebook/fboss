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
};

} // namespace facebook::fboss
