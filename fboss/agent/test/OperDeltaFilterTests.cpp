// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

using namespace ::testing;

#include "fboss/agent/Utils.h"

#include "fboss/agent/FsdbHelper.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

TEST(OperDeltaFilterTests, FilterOperDelta) {
  auto cfg = testConfigA();
  auto handle = createTestHandle(&cfg);

  utility::RSWRouteScaleGenerator generator(handle->getSw()->getState());
  handle->getSw()->updateStateBlocking(
      "update 1", [=](const std::shared_ptr<SwitchState>& state) {
        return generator.resolveNextHops(state);
      });
  auto rid = RouterID(0);
  auto client = ClientID::BGPD;
  auto routeChunks = generator.getThriftRoutes();
  auto updater = handle->getSw()->getRouteUpdater();
  for (const auto& routeChunk : routeChunks) {
    std::for_each(
        routeChunk.begin(),
        routeChunk.end(),
        [&updater, client, rid](const auto& route) {
          updater.addRoute(rid, client, route);
        });
    updater.program();
  }
  auto state = handle->getSw()->getState();

  StateDelta stateDelta(std::make_shared<SwitchState>(), state);

  {
    OperDeltaFilter filter(SwitchID(0));
    auto filteredOperDelta =
        filter.filterWithSwitchStateRootPath(stateDelta.getOperDelta());
    EXPECT_TRUE(filteredOperDelta.has_value());
    EXPECT_EQ(*filteredOperDelta, stateDelta.getOperDelta());
  }

  {
    OperDeltaFilter filter(SwitchID(12));
    auto filteredOperDelta =
        filter.filterWithSwitchStateRootPath(stateDelta.getOperDelta());
    EXPECT_FALSE(filteredOperDelta.has_value());
  }
}

} // namespace facebook::fboss
