/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/RouteUpdateLogger.h"
#include "fboss/agent/state/DeltaFunctions.h"

namespace facebook::fboss {

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
          sw,
          std::make_unique<RouteLogger<folly::IPAddressV4>>(),
          std::make_unique<RouteLogger<folly::IPAddressV6>>(),
          std::make_unique<MplsRouteLogger>()) {}

RouteUpdateLogger::RouteUpdateLogger(
    SwSwitch* sw,
    std::unique_ptr<RouteLogger<folly::IPAddressV4>> routeLoggerV4,
    std::unique_ptr<RouteLogger<folly::IPAddressV6>> routeLoggerV6,
    std::unique_ptr<MplsRouteLogger> mplsRouteLogger)
    : AutoRegisterStateObserver(sw, "RouteUpdateLogger"),
      routeLoggerV4_(std::move(routeLoggerV4)),
      routeLoggerV6_(std::move(routeLoggerV6)),
      mplsRouteLogger_(std::move(mplsRouteLogger)) {}

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

  auto* mplsRouteLogger = mplsRouteLogger_.get();
  CHECK(mplsRouteLogger);
  const auto labelTracker = labelTracker_.rlock();

  DeltaFunctions::forEachChanged(
      delta.getLabelForwardingInformationBaseDelta(),
      [mplsRouteLogger, &labelTracker](
          const auto& oldEntry, const auto& newEntry) {
        std::set<std::string> identifiers;
        labelTracker->getIdentifiersForLabel(oldEntry->getID(), identifiers);
        if (identifiers.empty()) {
          return;
        }
        mplsRouteLogger->logChangedRoute(
            oldEntry,
            newEntry,
            std::vector<std::string>(identifiers.begin(), identifiers.end()));
      },
      [mplsRouteLogger, &labelTracker](const auto& addedEntry) {
        std::set<std::string> identifiers;
        labelTracker->getIdentifiersForLabel(addedEntry->getID(), identifiers);
        if (identifiers.empty()) {
          return;
        }
        mplsRouteLogger->logAddedRoute(
            addedEntry,
            std::vector<std::string>(identifiers.begin(), identifiers.end()));
      },
      [mplsRouteLogger, &labelTracker](const auto& removedEntry) {
        std::set<std::string> identifiers;
        labelTracker->getIdentifiersForLabel(
            removedEntry->getID(), identifiers);
        if (identifiers.empty()) {
          return;
        }
        mplsRouteLogger->logRemovedRoute(
            removedEntry,
            std::vector<std::string>(identifiers.begin(), identifiers.end()));
      });
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

std::vector<RouteUpdateLoggingInstance> RouteUpdateLogger::getTrackedPrefixes()
    const {
  return prefixTracker_.getTrackedPrefixes();
}

template <typename AddrT>
void RouteLogger<AddrT>::logAddedRoute(
    const std::shared_ptr<Route<AddrT>>& newRoute,
    const std::vector<std::string>& identifiers) {
  for (const auto& identifier : identifiers) {
    auto newRouteStr = newRoute->str();
    updateLogHandler_->log(
        UpdateLogHandler::UPDATE_TYPE::ADD,
        identifier,
        "route",
        nullptr,
        &newRouteStr);
  }
}

template <typename AddrT>
void RouteLogger<AddrT>::logChangedRoute(
    const std::shared_ptr<Route<AddrT>>& oldRoute,
    const std::shared_ptr<Route<AddrT>>& newRoute,
    const std::vector<std::string>& identifiers) {
  for (const auto& identifier : identifiers) {
    auto oldRouteStr = oldRoute->str();
    auto newRouteStr = newRoute->str();
    updateLogHandler_->log(
        UpdateLogHandler::UPDATE_TYPE::CHANGE,
        identifier,
        "route",
        &oldRouteStr,
        &newRouteStr);
  }
}

template <typename AddrT>
void RouteLogger<AddrT>::logRemovedRoute(
    const std::shared_ptr<Route<AddrT>>& oldRoute,
    const std::vector<std::string>& identifiers) {
  for (const auto& identifier : identifiers) {
    auto oldRouteStr = oldRoute->str();
    updateLogHandler_->log(
        UpdateLogHandler::UPDATE_TYPE::DELETE,
        identifier,
        "route",
        &oldRouteStr,
        nullptr);
  }
}

void GlogUpdateLogHandler::log(
    UPDATE_TYPE operation,
    const std::string& identifier,
    const std::string& entityType,
    const std::string* oldEntity,
    const std::string* newEntity) {
  XLOG(INFO) << "[" << identifier << "] "
             << UpdateLogHandler::updateString(operation) << " " << entityType
             << "." << entityUpdateMessage(entityType, oldEntity, newEntity);
}

std::string GlogUpdateLogHandler::entityUpdateMessage(
    const std::string& entityType,
    const std::string* oldEntity,
    const std::string* newEntity) {
  if (oldEntity && newEntity) {
    return folly::to<std::string>(
        "old ",
        entityType,
        ": ",
        *oldEntity,
        "-> new ",
        entityType,
        ": ",
        *newEntity);
  } else if (oldEntity) {
    return *oldEntity;
  } else {
    return *newEntity;
  }
}

void MplsRouteLogger::logAddedRoute(
    const std::shared_ptr<LabelForwardingEntry>& newRoute,
    const std::vector<std::string>& identifiers) {
  for (const auto& identifier : identifiers) {
    auto newRouteStr = newRoute->str();
    updateLogHandler_->log(
        UpdateLogHandler::UPDATE_TYPE::ADD,
        identifier,
        "label fib entry entry",
        nullptr,
        &newRouteStr);
  }
}

void MplsRouteLogger::logChangedRoute(
    const std::shared_ptr<LabelForwardingEntry>& oldRoute,
    const std::shared_ptr<LabelForwardingEntry>& newRoute,
    const std::vector<std::string>& identifiers) {
  for (const auto& identifier : identifiers) {
    auto oldRouteStr = oldRoute->str();
    auto newRouteStr = newRoute->str();
    updateLogHandler_->log(
        UpdateLogHandler::UPDATE_TYPE::CHANGE,
        identifier,
        "label fib entry entry",
        &oldRouteStr,
        &newRouteStr);
  }
}

void MplsRouteLogger::logRemovedRoute(
    const std::shared_ptr<LabelForwardingEntry>& oldRoute,
    const std::vector<std::string>& identifiers) {
  for (const auto& identifier : identifiers) {
    auto oldRouteStr = oldRoute->str();
    updateLogHandler_->log(
        UpdateLogHandler::UPDATE_TYPE::DELETE,
        identifier,
        "label fib entry entry",
        &oldRouteStr,
        nullptr);
  }
}

void RouteUpdateLogger::startLoggingForLabel(
    LabelForwardingEntry::Label label,
    const std::string& identifier) {
  labelTracker_.wlock()->track(label, identifier);
}

void RouteUpdateLogger::stopLoggingForLabel(
    LabelForwardingEntry::Label label,
    const std::string& identifier) {
  labelTracker_.wlock()->untrack(label, identifier);
}

void RouteUpdateLogger::stopLabelLoggingForIdentifier(
    const std::string& identifier) {
  labelTracker_.wlock()->untrack(identifier);
}

LabelsTracker::TrackedLabelsInfo RouteUpdateLogger::gettTrackedLabels() const {
  return labelTracker_.rlock()->getTrackedLabelsInfo();
}

void LabelsTracker::track(
    LabelForwardingEntry::Label label,
    const std::string& identifier) {
  auto emplaced =
      label2Ids_.emplace(label, boost::container::flat_set<std::string>());
  auto itr = emplaced.first;
  itr->second.emplace(identifier);
}

void LabelsTracker::untrack(
    LabelForwardingEntry::Label label,
    const std::string& identifier) {
  auto labelItr = label2Ids_.find(label);
  if (labelItr == label2Ids_.end()) {
    return;
  }
  auto idItr = labelItr->second.find(identifier);
  if (idItr == labelItr->second.end()) {
    return;
  }
  labelItr->second.erase(idItr);
  if (labelItr->second.empty()) {
    label2Ids_.erase(labelItr);
  }
}

void LabelsTracker::untrack(const std::string& identifier) {
  auto itr = label2Ids_.begin();
  while (itr != label2Ids_.end()) {
    itr->second.erase(identifier);
    if (itr->second.empty()) {
      itr = label2Ids_.erase(itr);
    } else {
      itr++;
    }
  }
}

LabelsTracker::TrackedLabelsInfo LabelsTracker::getTrackedLabelsInfo() const {
  auto info = TrackedLabelsInfo();
  for (auto label2Id : label2Ids_) {
    for (auto id : label2Id.second) {
      info.emplace(std::make_pair(id, label2Id.first));
    }
  }
  return info;
}

void LabelsTracker::getIdentifiersForLabel(
    LabelForwardingEntry::Label label,
    std::set<std::string>& identifiers) const {
  auto label2IdsItr = label2Ids_.find(label);
  if (label2IdsItr != label2Ids_.end()) {
    for (auto id : label2IdsItr->second) {
      identifiers.insert(id);
    }
  }
  auto allIdsItr = label2Ids_.find(kAll);
  if (allIdsItr != label2Ids_.end()) {
    for (auto id : allIdsItr->second) {
      identifiers.insert(id);
    }
  }
}

} // namespace facebook::fboss
