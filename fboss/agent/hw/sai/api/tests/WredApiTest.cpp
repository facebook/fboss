/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/WredApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

class WredApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    wredApi = std::make_unique<WredApi>();
  }

  WredSaiId createWredProfile(
      const bool greenEnable,
      const sai_uint32_t greenMinThreshold,
      const sai_uint32_t greenMaxThreshold,
      const sai_ecn_mark_mode_t ecnMarkMode,
      const sai_uint32_t ecnGreenMinThreshold,
      const sai_uint32_t ecnGreenMaxThreshold) const {
    SaiWredTraits::Attributes::GreenEnable greenEnableAttribute{greenEnable};
    SaiWredTraits::Attributes::GreenMinThreshold greenMinThresholdAttribute{
        greenMinThreshold};
    SaiWredTraits::Attributes::GreenMaxThreshold greenMaxThresholdAttribute{
        greenMaxThreshold};
    SaiWredTraits::Attributes::EcnMarkMode ecnMarkModeAttribute{ecnMarkMode};
    SaiWredTraits::Attributes::EcnGreenMinThreshold
        ecnGreenMinThresholdAttribute{ecnGreenMinThreshold};
    SaiWredTraits::Attributes::EcnGreenMaxThreshold
        ecnGreenMaxThresholdAttribute{ecnGreenMaxThreshold};

    return wredApi->create<SaiWredTraits>(
        {greenEnableAttribute,
         greenMinThresholdAttribute,
         greenMaxThresholdAttribute,
         ecnMarkModeAttribute,
         ecnGreenMinThresholdAttribute,
         ecnGreenMaxThresholdAttribute},
        0);
  }

  sai_uint32_t kGreenMinThreshold() const {
    return 100;
  }

  sai_uint32_t kGreenMaxThreshold() const {
    return 200;
  }

  sai_uint32_t kEcnGreenMinThreshold() const {
    return 1000;
  }

  sai_uint32_t kEcnGreenMaxThreshold() const {
    return 2000;
  }

  void checkWredId(WredSaiId wredId) const {
    EXPECT_EQ(wredId, fs->wredManager.get(wredId).id);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<WredApi> wredApi;
};

TEST_F(WredApiTest, createWred) {
  auto wredId = createWredProfile(
      true,
      kGreenMinThreshold(),
      kGreenMaxThreshold(),
      SAI_ECN_MARK_MODE_NONE,
      0,
      0);

  checkWredId(wredId);

  EXPECT_TRUE(
      wredApi->getAttribute(wredId, SaiWredTraits::Attributes::GreenEnable{}));
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::GreenMinThreshold{}),
      kGreenMinThreshold());
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::GreenMaxThreshold{}),
      kGreenMaxThreshold());

  EXPECT_EQ(
      wredApi->getAttribute(wredId, SaiWredTraits::Attributes::EcnMarkMode{}),
      SAI_ECN_MARK_MODE_NONE);
}

TEST_F(WredApiTest, createEcn) {
  auto wredId = createWredProfile(
      false,
      0,
      0,
      SAI_ECN_MARK_MODE_GREEN,
      kEcnGreenMinThreshold(),
      kEcnGreenMaxThreshold());

  checkWredId(wredId);

  EXPECT_FALSE(
      wredApi->getAttribute(wredId, SaiWredTraits::Attributes::GreenEnable{}));

  EXPECT_EQ(
      wredApi->getAttribute(wredId, SaiWredTraits::Attributes::EcnMarkMode{}),
      SAI_ECN_MARK_MODE_GREEN);
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::EcnGreenMinThreshold{}),
      kEcnGreenMinThreshold());
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::EcnGreenMaxThreshold{}),
      kEcnGreenMaxThreshold());
}

TEST_F(WredApiTest, removeWredProfile) {
  auto wredId = createWredProfile(
      true,
      kGreenMinThreshold(),
      kGreenMaxThreshold(),
      SAI_ECN_MARK_MODE_GREEN,
      kEcnGreenMinThreshold(),
      kEcnGreenMaxThreshold());
  checkWredId(wredId);
  wredApi->remove(wredId);
}

TEST_F(WredApiTest, setWredAttributes) {
  auto wredId = createWredProfile(
      true,
      kGreenMinThreshold(),
      kGreenMaxThreshold(),
      SAI_ECN_MARK_MODE_NONE,
      0,
      0);

  checkWredId(wredId);

  EXPECT_TRUE(
      wredApi->getAttribute(wredId, SaiWredTraits::Attributes::GreenEnable{}));
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::GreenMinThreshold{}),
      kGreenMinThreshold());
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::GreenMaxThreshold{}),
      kGreenMaxThreshold());

  EXPECT_EQ(
      wredApi->getAttribute(wredId, SaiWredTraits::Attributes::EcnMarkMode{}),
      SAI_ECN_MARK_MODE_NONE);

  wredApi->setAttribute(wredId, SaiWredTraits::Attributes::GreenEnable{false});
  wredApi->setAttribute(
      wredId, SaiWredTraits::Attributes::GreenMinThreshold{42});
  wredApi->setAttribute(
      wredId, SaiWredTraits::Attributes::GreenMaxThreshold{42});

  EXPECT_FALSE(
      wredApi->getAttribute(wredId, SaiWredTraits::Attributes::GreenEnable{}));
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::GreenMinThreshold{}),
      42);
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::GreenMaxThreshold{}),
      42);
}

TEST_F(WredApiTest, setEcnAttributes) {
  auto wredId = createWredProfile(
      false,
      0,
      0,
      SAI_ECN_MARK_MODE_GREEN,
      kEcnGreenMinThreshold(),
      kEcnGreenMaxThreshold());

  checkWredId(wredId);

  EXPECT_FALSE(
      wredApi->getAttribute(wredId, SaiWredTraits::Attributes::GreenEnable{}));

  EXPECT_EQ(
      wredApi->getAttribute(wredId, SaiWredTraits::Attributes::EcnMarkMode{}),
      SAI_ECN_MARK_MODE_GREEN);
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::EcnGreenMinThreshold{}),
      kEcnGreenMinThreshold());
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::EcnGreenMaxThreshold{}),
      kEcnGreenMaxThreshold());

  wredApi->setAttribute(
      wredId, SaiWredTraits::Attributes::EcnMarkMode{SAI_ECN_MARK_MODE_ALL});
  wredApi->setAttribute(
      wredId, SaiWredTraits::Attributes::EcnGreenMinThreshold{42});
  wredApi->setAttribute(
      wredId, SaiWredTraits::Attributes::EcnGreenMaxThreshold{42});

  EXPECT_EQ(
      wredApi->getAttribute(wredId, SaiWredTraits::Attributes::EcnMarkMode{}),
      SAI_ECN_MARK_MODE_ALL);
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::EcnGreenMinThreshold{}),
      42);
  EXPECT_EQ(
      wredApi->getAttribute(
          wredId, SaiWredTraits::Attributes::EcnGreenMaxThreshold{}),
      42);
}
