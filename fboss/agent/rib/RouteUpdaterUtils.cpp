/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/rib/RouteUpdaterUtils.h"

#include <algorithm>
#include <iterator>

#include <boost/container/flat_set.hpp>

namespace facebook::fboss {
namespace {

bool haveSameInterfaceAndAddress(const NextHop& a, const NextHop& b) {
  return a.intfID() == b.intfID() && a.addr() == b.addr();
}

} // namespace

/*
 * RouteNextHopSet is ordered by interface ID and address first, so next hops
 * sharing those fields form contiguous groups. For each group containing a
 * primary, remove every backup; otherwise preserve the group unchanged. Other
 * attributes do not distinguish paths for this purpose because a primary and
 * backup using the same interface and address fail together.
 */
RouteNextHopSet removeBackupNextHopsWithMatchingPrimary(
    RouteNextHopSet nextHops) {
  // Most routes have no backup next hops, so avoid allocating a scratch
  // vector for the common case.
  if (std::none_of(nextHops.begin(), nextHops.end(), [](const auto& nextHop) {
        return nextHop.role() == NextHopRole::BACKUP;
      })) {
    return nextHops;
  }

  std::vector<NextHop> filteredNextHops;
  filteredNextHops.reserve(nextHops.size());

  for (auto groupBegin = nextHops.begin(); groupBegin != nextHops.end();) {
    const auto groupEnd = std::find_if_not(
        std::next(groupBegin), nextHops.end(), [&](const auto& nextHop) {
          return haveSameInterfaceAndAddress(*groupBegin, nextHop);
        });
    const bool hasPrimary =
        std::any_of(groupBegin, groupEnd, [](const auto& nextHop) {
          return nextHop.role() == NextHopRole::PRIMARY;
        });

    std::copy_if(
        groupBegin,
        groupEnd,
        std::back_inserter(filteredNextHops),
        [hasPrimary](const auto& nextHop) {
          return !hasPrimary || nextHop.role() != NextHopRole::BACKUP;
        });
    groupBegin = groupEnd;
  }

  if (filteredNextHops.size() == nextHops.size()) {
    return nextHops;
  }
  return RouteNextHopSet(
      boost::container::ordered_unique_range,
      std::make_move_iterator(filteredNextHops.begin()),
      std::make_move_iterator(filteredNextHops.end()));
}

} // namespace facebook::fboss
