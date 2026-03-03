// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/Srv6Api.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"

#include <folly/logging/xlog.h>

#include <gtest/gtest.h>

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
using namespace facebook::fboss;

class Srv6ApiTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    sai_api_initialize(0, nullptr);
    srv6Api = std::make_unique<Srv6Api>();
  }

  Srv6SidListSaiId createSrv6SidList(sai_int32_t type) {
    SaiSrv6SidListTraits::Attributes::Type typeAttr{type};
    SaiSrv6SidListTraits::CreateAttributes a{
        typeAttr, std::nullopt, std::nullopt};
    return srv6Api->create<SaiSrv6SidListTraits>(a, 0);
  }

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<Srv6Api> srv6Api;
};

TEST_F(Srv6ApiTest, createSrv6SidList) {
  auto id = createSrv6SidList(SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
  EXPECT_EQ(
      srv6Api->getAttribute(id, SaiSrv6SidListTraits::Attributes::Type{}),
      SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
}

TEST_F(Srv6ApiTest, removeSrv6SidList) {
  auto id = createSrv6SidList(SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
  srv6Api->remove(id);
  EXPECT_THROW(
      srv6Api->getAttribute(id, SaiSrv6SidListTraits::Attributes::Type{}),
      std::exception);
}

TEST_F(Srv6ApiTest, setSrv6SidListNextHop) {
  auto id = createSrv6SidList(SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
  srv6Api->setAttribute(id, SaiSrv6SidListTraits::Attributes::NextHopId{42});
  EXPECT_EQ(
      srv6Api->getAttribute(id, SaiSrv6SidListTraits::Attributes::NextHopId{}),
      42);
}

TEST_F(Srv6ApiTest, getSrv6SidListAttributes) {
  auto id = createSrv6SidList(SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
  EXPECT_EQ(
      srv6Api->getAttribute(id, SaiSrv6SidListTraits::Attributes::Type{}),
      SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED);
  EXPECT_EQ(
      srv6Api->getAttribute(id, SaiSrv6SidListTraits::Attributes::NextHopId{}),
      SAI_NULL_OBJECT_ID);
}
#endif
