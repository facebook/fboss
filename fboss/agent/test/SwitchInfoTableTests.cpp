// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/SwitchInfoTable.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <gtest/gtest.h>
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;

template <typename SwitchTypeT>
class SwitchInfoTableTest : public testing::Test {
 public:
  static auto constexpr switchType = SwitchTypeT::switchType;
  void SetUp() override {
    std::map<int64_t, cfg::SwitchInfo> switchInfos;
    for (auto i = 0; i < 2; ++i) {
      cfg::SwitchInfo info;
      info.switchType() = switchType;
      info.switchIndex() = i;
      switchInfos.insert({i, info});
    }
    infoTable = std::make_unique<SwitchInfoTable>(switchInfos);
  };
  std::unique_ptr<SwitchInfoTable> infoTable;
  bool isVoq() const {
    return this->switchType == cfg::SwitchType::VOQ;
  }
  bool isFabric() const {
    return this->switchType == cfg::SwitchType::FABRIC;
  }
  bool isNpu() const {
    return this->switchType == cfg::SwitchType::NPU;
  }
};

TYPED_TEST_SUITE(SwitchInfoTableTest, SwitchTypeTestTypes);

TYPED_TEST(SwitchInfoTableTest, getSwitchIds) {
  EXPECT_EQ(this->infoTable->getSwitchIDs().size(), 2);
  EXPECT_EQ(this->infoTable->getSwitchIdsOfType(this->switchType).size(), 2);
}

TYPED_TEST(SwitchInfoTableTest, getSwitchIndices) {
  EXPECT_EQ(
      this->infoTable->getSwitchIndicesOfType(this->switchType).size(), 2);
}
