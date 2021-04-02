/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/api/MacsecApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

using namespace facebook::fboss;

namespace {
constexpr sai_object_id_t fakeSwitchId = 0;
} // namespace

class MacsecApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    macsecApi = std::make_unique<MacsecApi>();
    ingressMacsec = createMacsec(SAI_MACSEC_DIRECTION_INGRESS);
    egressMacsec = createMacsec(SAI_MACSEC_DIRECTION_EGRESS);
  }

  MacsecSaiId createMacsec(sai_macsec_direction_t direction) {
    SaiMacsecTraits::CreateAttributes a{direction, std::nullopt};
    return macsecApi->create<SaiMacsecTraits>(a, fakeSwitchId);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<MacsecApi> macsecApi;
  MacsecSaiId ingressMacsec;
  MacsecSaiId egressMacsec;
};

TEST_F(MacsecApiTest, setGetPhysicalBypass) {
  SaiMacsecTraits::Attributes::PhysicalBypass physBypass{true};
  macsecApi->setAttribute(ingressMacsec, physBypass);

  SaiMacsecTraits::Attributes::PhysicalBypass blank{false};
  EXPECT_TRUE(macsecApi->getAttribute(ingressMacsec, blank));

  physBypass = false;
  macsecApi->setAttribute(ingressMacsec, physBypass);

  EXPECT_FALSE(macsecApi->getAttribute(ingressMacsec, blank));
}
