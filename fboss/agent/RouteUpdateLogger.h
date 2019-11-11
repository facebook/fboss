/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/IPAddress.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/RouteUpdateLoggingPrefixTracker.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/state/StateDelta.h"

#include <memory>
#include <vector>

namespace facebook {
namespace fboss {

template <typename RouteT>
class RouteLoggerBase {
 public:
  virtual ~RouteLoggerBase() {}
  virtual void logAddedRoute(
      const std::shared_ptr<RouteT>& newRoute,
      const std::vector<std::string>& identifiers) = 0;
  virtual void logChangedRoute(
      const std::shared_ptr<RouteT>& oldRoute,
      const std::shared_ptr<RouteT>& newRoute,
      const std::vector<std::string>& identifiers) = 0;
  virtual void logRemovedRoute(
      const std::shared_ptr<RouteT>& oldRoute,
      const std::vector<std::string>& identifiers) = 0;
};

struct UpdateLogHandler {
  enum UPDATE_TYPE { ADD, CHANGE, DELETE };
  virtual ~UpdateLogHandler() {}
  static std::string updateString(UPDATE_TYPE type) {
    switch (type) {
      case ADD:
        return "Added";
      case CHANGE:
        return "Changed";
      case DELETE:
        return "Deleted";
    }
  }

  virtual void log(
      UPDATE_TYPE operation,
      const std::string& identifier,
      const std::string& message,
      const std::string* oldEntity,
      const std::string* newEntity) = 0;

  static std::unique_ptr<UpdateLogHandler> get();
};

struct GlogUpdateLogHandler : UpdateLogHandler {
  void log(
      UPDATE_TYPE operation,
      const std::string& identifier,
      const std::string& message,
      const std::string* oldEntity,
      const std::string* newEntity) override;

 private:
  std::string entityUpdateMessage(
      const std::string& entityType,
      const std::string* oldEntity,
      const std::string* newEntity);
};

template <typename AddrT>
class RouteLogger : public RouteLoggerBase<Route<AddrT>> {
 public:
  RouteLogger() : updateLogHandler_(UpdateLogHandler::get()) {}

  void logAddedRoute(
      const std::shared_ptr<Route<AddrT>>& newRoute,
      const std::vector<std::string>& identifiers) override;
  void logChangedRoute(
      const std::shared_ptr<Route<AddrT>>& oldRoute,
      const std::shared_ptr<Route<AddrT>>& newRoute,
      const std::vector<std::string>& identifiers) override;
  void logRemovedRoute(
      const std::shared_ptr<Route<AddrT>>& oldRoute,
      const std::vector<std::string>& identifiers) override;

 private:
  std::unique_ptr<UpdateLogHandler> updateLogHandler_;
};

/*
 * Log changes to the routes in SwitchState.
 * Allow subscription to a prefix. When a route to a subscribed prefix
 * (or more specific location with that prefix) is added, removed, or
 * changes, log that information. The logger is pluggable, but by default
 * we use GLOG.
 */
class RouteUpdateLogger : public AutoRegisterStateObserver {
  // TODO(pshaikh): rename RouteUpdateLogger to FibUpdateObserver
 public:
  explicit RouteUpdateLogger(SwSwitch* sw);
  RouteUpdateLogger(
      SwSwitch* sw,
      std::unique_ptr<RouteLogger<folly::IPAddressV4>> routeLoggerV4,
      std::unique_ptr<RouteLogger<folly::IPAddressV6>> routeLoggerV6);

  ~RouteUpdateLogger() override = default;

  void stateUpdated(const StateDelta& delta) override;
  void startLoggingForPrefix(const RouteUpdateLoggingInstance& req);
  void stopLoggingForPrefix(
      const folly::IPAddress& network,
      uint8_t mask,
      const std::string& identifier);
  void stopLoggingForIdentifier(const std::string& identifier);
  std::vector<RouteUpdateLoggingInstance> getTrackedPrefixes() const;

  RouteLogger<folly::IPAddressV4>* getRouteLoggerV4() const {
    return routeLoggerV4_.get();
  }
  RouteLogger<folly::IPAddressV6>* getRouteLoggerV6() const {
    return routeLoggerV6_.get();
  }

 private:
  RouteUpdateLoggingPrefixTracker prefixTracker_;
  std::unique_ptr<RouteLogger<folly::IPAddressV4>> routeLoggerV4_;
  std::unique_ptr<RouteLogger<folly::IPAddressV6>> routeLoggerV6_;
};

} // namespace fboss
} // namespace facebook
