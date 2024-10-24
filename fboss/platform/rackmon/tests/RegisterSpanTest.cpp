// Copyright 2024-present Facebook. All Rights Reserved.
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "Register.h"

using namespace std;
using namespace testing;
using namespace rackmon;

class RegisterSpanTest : public ::testing::Test {
 protected:
  RegisterDescriptor desc[4];
  std::vector<RegisterStore> regs;
  void SetUp() override {
    desc[0].begin = 4;
    desc[0].length = 2;
    desc[0].interval = 1;
    desc[1].begin = 6;
    desc[1].length = 4;
    desc[1].interval = 1;
    desc[2].begin = 10;
    desc[2].length = 2;
    desc[2].interval = 2;
    desc[3].begin = 19;
    desc[3].length = 1;
    regs.emplace_back(desc[0]);
    regs.emplace_back(desc[1]);
    regs.emplace_back(desc[2]);
    regs.emplace_back(desc[3]);
  }
};

//--------------------------------------------------------
// RegisterSpanTests
//--------------------------------------------------------

TEST_F(RegisterSpanTest, BasicCreate) {
  RegisterStoreSpan span(&regs[0]);
  ASSERT_EQ(span.length(), 2);
  ASSERT_TRUE(span.addRegister(&regs[1]));
  ASSERT_EQ(span.length(), 6);
  ASSERT_FALSE(span.addRegister(&regs[2]));
  ASSERT_FALSE(span.addRegister(&regs[3]));
  ASSERT_EQ(span.length(), 6);

  ASSERT_TRUE(span.reloadPending(1234));
  auto& flat = span.beginReloadSpan();
  ASSERT_EQ(flat.size(), 6);
  for (auto i = 0; i < 6; i++) {
    flat[i] = i;
  }
  span.endReloadSpan(1234);
  ASSERT_EQ(regs[0].back().timestamp, 1234);
  ASSERT_EQ(regs[1].back().timestamp, 1234);
  ASSERT_EQ(regs[0].back().value[0], 0);
  ASSERT_EQ(regs[0].back().value[1], 1);
  ASSERT_EQ(regs[1].back().value[0], 2);
  ASSERT_EQ(regs[1].back().value[1], 3);
  ASSERT_EQ(regs[1].back().value[2], 4);
  ASSERT_EQ(regs[1].back().value[3], 5);
}

TEST_F(RegisterSpanTest, spanListTest) {
  std::vector<RegisterStoreSpan> spanList{};
  for (auto& reg : regs) {
    ASSERT_TRUE(RegisterStoreSpan::buildRegisterSpanList(spanList, reg));
  }
  ASSERT_EQ(spanList.size(), 3);
  ASSERT_EQ(spanList[0].getSpanAddress(), 4);
  ASSERT_EQ(spanList[0].length(), 6);
  ASSERT_EQ(spanList[1].getSpanAddress(), 10);
  ASSERT_EQ(spanList[1].length(), 2);
  ASSERT_EQ(spanList[2].getSpanAddress(), 19);
  ASSERT_EQ(spanList[2].length(), 1);
  spanList.clear();
  regs[3].disable();
  ASSERT_TRUE(RegisterStoreSpan::buildRegisterSpanList(spanList, regs[0]));
  ASSERT_TRUE(RegisterStoreSpan::buildRegisterSpanList(spanList, regs[1]));
  ASSERT_TRUE(RegisterStoreSpan::buildRegisterSpanList(spanList, regs[2]));
  ASSERT_FALSE(RegisterStoreSpan::buildRegisterSpanList(spanList, regs[3]));
  ASSERT_EQ(spanList.size(), 2);
  ASSERT_EQ(spanList[0].getSpanAddress(), 4);
  ASSERT_EQ(spanList[0].length(), 6);
  ASSERT_EQ(spanList[1].getSpanAddress(), 10);
  ASSERT_EQ(spanList[1].length(), 2);
}

TEST_F(RegisterSpanTest, smallSpanListTest) {
  std::vector<RegisterStoreSpan> spanList{};
  for (auto& reg : regs) {
    ASSERT_TRUE(RegisterStoreSpan::buildRegisterSpanList(spanList, reg, 4));
  }
  ASSERT_EQ(spanList.size(), 4);
  ASSERT_EQ(spanList[0].getSpanAddress(), 4);
  ASSERT_EQ(spanList[0].length(), 2);
  ASSERT_EQ(spanList[1].getSpanAddress(), 6);
  ASSERT_EQ(spanList[1].length(), 4);
  ASSERT_EQ(spanList[2].getSpanAddress(), 10);
  ASSERT_EQ(spanList[2].length(), 2);
  ASSERT_EQ(spanList[3].getSpanAddress(), 19);
  ASSERT_EQ(spanList[3].length(), 1);
}
