/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/RouteTestUtils.h"
#include "fboss/agent/RouteUpdateWrapper.h"

namespace {
void programRoutesImpl(
    facebook::fboss::RouteUpdateWrapper& routeUpdater,
    facebook::fboss::RouterID rid,
    facebook::fboss::ClientID client,
    const facebook::fboss::utility::RouteDistributionGenerator::
        ThriftRouteChunks& routeChunks,
    bool add) {
  for (const auto& routeChunk : routeChunks) {
    std::for_each(
        routeChunk.begin(),
        routeChunk.end(),
        [client, rid, add, &routeUpdater](const auto& route) {
          if (add) {
            routeUpdater.addRoute(rid, client, route);
          } else {
            routeUpdater.delRoute(rid, *route.dest(), client);
          }
        });
    routeUpdater.program();
  }
}
} // namespace

namespace facebook::fboss::utility {

void programRoutes(
    RouteUpdateWrapper& routeUpdater,
    RouterID rid,
    ClientID client,
    const utility::RouteDistributionGenerator::ThriftRouteChunks& routeChunks) {
  programRoutesImpl(routeUpdater, rid, client, routeChunks, true /* add*/);
}

void unprogramRoutes(
    RouteUpdateWrapper& routeUpdater,
    RouterID rid,
    ClientID client,
    const utility::RouteDistributionGenerator::ThriftRouteChunks& routeChunks) {
  programRoutesImpl(routeUpdater, rid, client, routeChunks, false /* del*/);
}

} // namespace facebook::fboss::utility
