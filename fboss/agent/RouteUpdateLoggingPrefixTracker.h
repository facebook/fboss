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

#include <folly/Synchronized.h>
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/lib/RadixTree.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace facebook::fboss {

/* All the information regarding a prefix for which we have
 * subscribed for logging.
 * prefix: the prefix we are logging route changes for
 * identifier: an extra name to allow several different users to
 *             independently turn logging on and off for the same prefixes
 * exact: whether or not we require an exact match or permit a more specific
 *      destination that matches the prefix.
 */
struct RouteUpdateLoggingInstance {
  RouteUpdateLoggingInstance(
      const RoutePrefix<folly::IPAddress>& prefix,
      const std::string& identifier,
      bool exact);
  RoutePrefix<folly::IPAddress> prefix;
  std::string identifier;
  bool exact;
  std::string str() const;
};

/*
 * Keep track of network prefixes that the agent will
 * log route updates for.
 *
 * All the methods in this class are thread safe.
 */
class RouteUpdateLoggingPrefixTracker {
 public:
  ~RouteUpdateLoggingPrefixTracker() {}
  /*
   * Start tracking a prefix. Will overwrite existing exact-ness settings.
   * i.e. for a single identifier, if we track expecting exact matches, then
   * track allowing for longest match, tracking() will use longest match.
   */
  void track(const RouteUpdateLoggingInstance& req);
  // Stop a particular tracking instance
  void stopTracking(
      const RoutePrefix<folly::IPAddress>& prefix,
      const std::string& identifier);
  // Stop tracking all the prefixes tracked with this identifier
  void stopTracking(const std::string& identifier);
  std::vector<RouteUpdateLoggingInstance> getTrackedPrefixes() const;

  /* Returns whether or not the prefix is tracked for logging.
   * Will also populate identifiers with all of the identifiers that
   * tracking for this prefix was turned on with.
   */
  template <typename AddrT>
  bool tracking(
      const RoutePrefix<AddrT>& prefix,
      std::vector<std::string>& identifiers) const {
    folly::IPAddress addr{prefix.network};
    RoutePrefix<folly::IPAddress> p{addr, prefix.mask};
    return trackingImpl(p, identifiers);
  }

 private:
  bool trackingImpl(
      const RoutePrefix<folly::IPAddress>& prefix,
      std::vector<std::string>& identifiers) const;
  folly::Synchronized<std::unordered_map<
      std::string,
      network::RadixTree<folly::IPAddress, RouteUpdateLoggingInstance>>>
      trackedPrefixes_;
};

} // namespace facebook::fboss
