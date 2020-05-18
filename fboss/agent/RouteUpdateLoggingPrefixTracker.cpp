/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "RouteUpdateLoggingPrefixTracker.h"
#include <folly/logging/xlog.h>

namespace facebook::fboss {

RouteUpdateLoggingInstance::RouteUpdateLoggingInstance(
    const RoutePrefix<folly::IPAddress>& prefix,
    const std::string& identifier,
    bool exact)
    : prefix(prefix), identifier(identifier), exact(exact) {}

std::string RouteUpdateLoggingInstance::str() const {
  return folly::sformat(
      "{} {} {}", prefix.str(), identifier, exact ? "exact" : "longest-match");
}

void RouteUpdateLoggingPrefixTracker::track(
    const RouteUpdateLoggingInstance& req) {
  XLOG(INFO) << "Tracking " << req.str();
  SYNCHRONIZED(trackedPrefixes_) {
    auto found = trackedPrefixes_[req.identifier].insert(
        req.prefix.network, req.prefix.mask, req);
    // Use the most recently set configuration
    if (!found.second) {
      found.first.value() = req;
    }
  }
}

// stop tracking a particular requested prefix
void RouteUpdateLoggingPrefixTracker::stopTracking(
    const RoutePrefix<folly::IPAddress>& prefix,
    const std::string& identifier) {
  XLOG(INFO) << "Stop tracking " << prefix.str() << " " << identifier;
  SYNCHRONIZED(trackedPrefixes_) {
    auto itr = trackedPrefixes_.find(identifier);
    if (itr == trackedPrefixes_.end()) {
      return;
    }
    itr->second.erase(prefix.network, prefix.mask);
  }
}

// stop tracking all the routes with the given identifier
void RouteUpdateLoggingPrefixTracker::stopTracking(
    const std::string& identifier) {
  XLOG(INFO) << "Stop tracking all prefixes for " << identifier;
  SYNCHRONIZED(trackedPrefixes_) {
    trackedPrefixes_.erase(identifier);
  }
}

bool RouteUpdateLoggingPrefixTracker::trackingImpl(
    const RoutePrefix<folly::IPAddress>& prefix,
    std::vector<std::string>& identifiers) const {
  identifiers.clear();
  SYNCHRONIZED_CONST(trackedPrefixes_) {
    for (const auto& prefixes : trackedPrefixes_) {
      const auto match =
          prefixes.second.longestMatch(prefix.network, prefix.mask);
      if (match != prefixes.second.end()) {
        if (!match.value().exact) {
          identifiers.push_back(prefixes.first);
          continue;
        }
        if (match.value().prefix == prefix) {
          identifiers.push_back(prefixes.first);
        }
      }
    }
  }
  return (identifiers.size() > 0);
}

std::vector<RouteUpdateLoggingInstance>
RouteUpdateLoggingPrefixTracker::getTrackedPrefixes() const {
  std::vector<RouteUpdateLoggingInstance> allPrefixes;
  SYNCHRONIZED_CONST(trackedPrefixes_) {
    for (const auto& prefixes : trackedPrefixes_) {
      for (const auto& itr : prefixes.second) {
        allPrefixes.push_back(itr->value());
      }
    }
  }
  return allPrefixes;
}

} // namespace facebook::fboss
