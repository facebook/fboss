/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "RouteUpdateLogger.h"
#include "fboss/agent/state/DeltaFunctions.h"

namespace facebook {
namespace fboss {

namespace {
template <typename AddrT>
void handleChangedRoute(
    const RouteUpdateLoggingPrefixTracker& tracker,
    const std::unique_ptr<RouteLogger<AddrT>>& logger,
    const std::shared_ptr<Route<AddrT>>& oldRoute,
    const std::shared_ptr<Route<AddrT>>& newRoute) {
  std::vector<std::string> matchedIdentifiers;
  auto prefix = oldRoute->prefix();
  if (tracker.tracking(prefix, matchedIdentifiers)) {
    logger->logChangedRoute(oldRoute, newRoute, matchedIdentifiers);
  }
}

template <typename AddrT>
void handleRemovedRoute(
    const RouteUpdateLoggingPrefixTracker& tracker,
    const std::unique_ptr<RouteLogger<AddrT>>& logger,
    const std::shared_ptr<Route<AddrT>>& oldRoute) {
  std::vector<std::string> matchedIdentifiers;
  auto prefix = oldRoute->prefix();
  if (tracker.tracking(prefix, matchedIdentifiers)) {
    logger->logRemovedRoute(oldRoute, matchedIdentifiers);
  }
}

template <typename AddrT>
void handleAddedRoute(
    const RouteUpdateLoggingPrefixTracker& tracker,
    const std::unique_ptr<RouteLogger<AddrT>>& logger,
    const std::shared_ptr<Route<AddrT>>& newRoute) {
  std::vector<std::string> matchedIdentifiers;
  auto prefix = newRoute->prefix();
  if (tracker.tracking(prefix, matchedIdentifiers)) {
    logger->logAddedRoute(newRoute, matchedIdentifiers);
  }
}
} // anonymous namespace

RouteUpdateLogger::RouteUpdateLogger(SwSwitch* sw)
    : RouteUpdateLogger(
          sw, getDefaultV4RouteLogger(), getDefaultV6RouteLogger()) {}

RouteUpdateLogger::RouteUpdateLogger(
    SwSwitch* sw,
    std::unique_ptr<RouteLogger<folly::IPAddressV4>> routeLoggerV4,
    std::unique_ptr<RouteLogger<folly::IPAddressV6>> routeLoggerV6)
    : AutoRegisterStateObserver(sw, "RouteUpdateLogger"),
      routeLoggerV4_(std::move(routeLoggerV4)),
      routeLoggerV6_(std::move(routeLoggerV6)) {}

void RouteUpdateLogger::stateUpdated(const StateDelta& delta) {
  for (const auto& rtDelta : delta.getRouteTablesDelta()) {
    DeltaFunctions::forEachChanged(
        rtDelta.getRoutesV4Delta(),
        &handleChangedRoute<folly::IPAddressV4>,
        &handleAddedRoute<folly::IPAddressV4>,
        &handleRemovedRoute<folly::IPAddressV4>,
        prefixTracker_,
        routeLoggerV4_);
    DeltaFunctions::forEachChanged(
        rtDelta.getRoutesV6Delta(),
        &handleChangedRoute<folly::IPAddressV6>,
        &handleAddedRoute<folly::IPAddressV6>,
        &handleRemovedRoute<folly::IPAddressV6>,
        prefixTracker_,
        routeLoggerV6_);
  }
}

void RouteUpdateLogger::startLoggingForPrefix(
    const RouteUpdateLoggingInstance& req) {
  prefixTracker_.track(req);
}

void RouteUpdateLogger::stopLoggingForPrefix(
    const folly::IPAddress& network,
    uint8_t mask,
    const std::string& identifier) {
  RoutePrefix<folly::IPAddress> prefix{network, mask};
  prefixTracker_.stopTracking(prefix, identifier);
}

void RouteUpdateLogger::stopLoggingForIdentifier(
    const std::string& identifier) {
  prefixTracker_.stopTracking(identifier);
}

std::vector<RouteUpdateLoggingInstance>
RouteUpdateLogger::getTrackedPrefixes() const {
  return prefixTracker_.getTrackedPrefixes();
}

}} // facebook::fboss
