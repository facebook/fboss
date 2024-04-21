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
    infoTable = std::make_unique<SwitchInfoTable>(switchInfo());
  };
  std::map<int64_t, cfg::SwitchInfo> switchInfo() const {
    std::map<int64_t, cfg::SwitchInfo> switchInfos;
    for (auto i = 0; i < 2; ++i) {
      cfg::SwitchInfo info;
      info.switchType() = switchType;
      info.switchIndex() = i;
      switchInfos.insert({i, info});
    }
    return switchInfos;
  }
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
  cfg::SwitchType otherSwitchType() const {
    switch (switchType) {
      case cfg::SwitchType::NPU:
        return cfg::SwitchType::VOQ;
      case cfg::SwitchType::VOQ:
        return cfg::SwitchType::NPU;
      case cfg::SwitchType::FABRIC:
        return cfg::SwitchType::VOQ;
      case cfg::SwitchType::PHY:
        return cfg::SwitchType::NPU;
    }
    CHECK(false) << " Unexpected switch type: " << static_cast<int>(switchType);
  }
};

TYPED_TEST_SUITE(SwitchInfoTableTest, SwitchTypeTestTypes);

TYPED_TEST(SwitchInfoTableTest, getSwitchIds) {
  EXPECT_EQ(this->infoTable->getSwitchIDs().size(), 2);
  EXPECT_EQ(this->infoTable->getSwitchIdsOfType(this->switchType).size(), 2);
  EXPECT_EQ(
      this->infoTable->getSwitchIdsOfType(this->otherSwitchType()).size(), 0);
}

TYPED_TEST(SwitchInfoTableTest, getSwitchIndices) {
  EXPECT_EQ(
      this->infoTable->getSwitchIndicesOfType(this->switchType).size(), 2);
  EXPECT_EQ(
      this->infoTable->getSwitchIndicesOfType(this->otherSwitchType()).size(),
      0);
}

TYPED_TEST(SwitchInfoTableTest, haveSwitchOfType) {
  if (this->isVoq()) {
    EXPECT_TRUE(this->infoTable->haveVoqSwitches());
    EXPECT_TRUE(this->infoTable->haveL3Switches());
    EXPECT_FALSE(this->infoTable->haveNpuSwitches());
  } else if (this->isNpu()) {
    EXPECT_TRUE(this->infoTable->haveNpuSwitches());
    EXPECT_TRUE(this->infoTable->haveL3Switches());
    EXPECT_FALSE(this->infoTable->haveVoqSwitches());
  } else {
    EXPECT_TRUE(this->infoTable->haveFabricSwitches());
    EXPECT_FALSE(this->infoTable->haveL3Switches());
    EXPECT_FALSE(this->infoTable->haveVoqSwitches());
  }
}

TYPED_TEST(SwitchInfoTableTest, vlansSupported) {
  if (this->isVoq()) {
    EXPECT_FALSE(this->infoTable->vlansSupported());
  } else if (this->isNpu()) {
    EXPECT_TRUE(this->infoTable->vlansSupported());
  } else {
    EXPECT_FALSE(this->infoTable->vlansSupported());
  }
}

TYPED_TEST(SwitchInfoTableTest, l3SwitchType) {
  if (this->isVoq()) {
    EXPECT_EQ(this->infoTable->l3SwitchType(), cfg::SwitchType::VOQ);
  } else if (this->isNpu()) {
    EXPECT_EQ(this->infoTable->l3SwitchType(), cfg::SwitchType::NPU);
  } else {
    EXPECT_THROW(this->infoTable->l3SwitchType(), FbossError);
  }
}

TYPED_TEST(SwitchInfoTableTest, getL3SwitchIDs) {
  if (this->isVoq() || this->isNpu()) {
    EXPECT_EQ(this->infoTable->getL3SwitchIDs().size(), 2);
  } else {
    EXPECT_EQ(this->infoTable->getL3SwitchIDs().size(), 0);
  }
}

TYPED_TEST(SwitchInfoTableTest, getSwitchIndex) {
  for (auto switchId : this->infoTable->getSwitchIDs()) {
    EXPECT_EQ(
        static_cast<int64_t>(switchId),
        this->infoTable->getSwitchIndexFromSwitchId(switchId));
  }
}

TYPED_TEST(SwitchInfoTableTest, getSwitchInfo) {
  for (auto switchId : this->infoTable->getSwitchIDs()) {
    EXPECT_EQ(
        static_cast<int64_t>(switchId),
        *this->infoTable->getSwitchInfo(switchId).switchIndex());
  }
}

TYPED_TEST(SwitchInfoTableTest, getSwitchIdToSwitchInfo) {
  EXPECT_EQ(this->infoTable->getSwitchIdToSwitchInfo().size(), 2);
}

TYPED_TEST(SwitchInfoTableTest, mixSwitchTypes) {
  auto switchInfos = this->switchInfo();
  cfg::SwitchInfo otherSwitch;
  otherSwitch.switchType() = this->otherSwitchType();
  otherSwitch.switchIndex() = 42;
  switchInfos.insert({42, otherSwitch});
  EXPECT_THROW(SwitchInfoTable{switchInfos}, FbossError);
}
