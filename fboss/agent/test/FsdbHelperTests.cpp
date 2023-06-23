// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

using namespace ::testing;

#include "fboss/agent/FsdbHelper.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/fsdb/common/Utils.h"

namespace facebook::fboss {

TEST(FsdbHelperTests, VerifyOperDelta) {
  auto cfg = testConfigA();
  auto handle = createTestHandle(&cfg);

  utility::HgridUuRouteScaleGenerator generator(handle->getSw()->getState());
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

  auto operDelta = fsdb::computeOperDelta(
      std::make_shared<SwitchState>(),
      state,
      fsdbAgentDataSwitchStateRootPath());
  auto& changes = *operDelta.changes();
  EXPECT_GT(changes.size(), 0);
  std::set<std::string> members{};
  for (auto& change : changes) {
    std::ostringstream path;
    auto& tokens = *change.path()->raw();
    members.insert(tokens[2]);
  }
  // route updates will lead to fib programming
  EXPECT_NE(members.find("fibsMap"), members.end());
}

} // namespace facebook::fboss
