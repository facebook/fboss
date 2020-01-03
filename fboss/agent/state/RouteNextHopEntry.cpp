/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "RouteNextHopEntry.h"

#include "fboss/agent/FbossError.h"

#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <numeric>

namespace {
constexpr auto kNexthops = "nexthops";
constexpr auto kAction = "action";
constexpr auto kAdminDistance = "adminDistance";
} // namespace

namespace facebook::fboss {

namespace util {

RouteNextHopSet toRouteNextHopSet(std::vector<NextHopThrift> const& nhs) {
  RouteNextHopSet rnhs;
  rnhs.reserve(nhs.size());
  for (auto const& nh : nhs) {
    rnhs.emplace(fromThrift(nh));
  }
  return rnhs;
}

std::vector<NextHopThrift> fromRouteNextHopSet(RouteNextHopSet const& nhs) {
  std::vector<NextHopThrift> nhts;
  nhts.reserve(nhs.size());
  for (const auto& nh : nhs) {
    nhts.emplace_back(nh.toThrift());
  }
  return nhts;
}

} // namespace util

RouteNextHopEntry::RouteNextHopEntry(NextHopSet nhopSet, AdminDistance distance)
    : adminDistance_(distance),
      action_(Action::NEXTHOPS),
      nhopSet_(std::move(nhopSet)) {
  if (nhopSet_.size() == 0) {
    throw FbossError("Empty nexthop set is passed to the RouteNextHopEntry");
  }
}

NextHopWeight RouteNextHopEntry::getTotalWeight() const {
  return totalWeight(getNextHopSet());
}

std::string RouteNextHopEntry::str() const {
  std::string result;
  switch (action_) {
    case Action::DROP:
      result = "DROP";
      break;
    case Action::TO_CPU:
      result = "To_CPU";
      break;
    case Action::NEXTHOPS:
      toAppend(getNextHopSet(), &result);
      break;
    default:
      CHECK(0);
      break;
  }
  result +=
      folly::to<std::string>(";admin=", static_cast<int32_t>(adminDistance_));
  return result;
}

bool operator==(const RouteNextHopEntry& a, const RouteNextHopEntry& b) {
  return (
      a.getAction() == b.getAction() and
      a.getNextHopSet() == b.getNextHopSet() and
      a.getAdminDistance() == b.getAdminDistance());
}

bool operator<(const RouteNextHopEntry& a, const RouteNextHopEntry& b) {
  if (a.getAdminDistance() != b.getAdminDistance()) {
    return a.getAdminDistance() < b.getAdminDistance();
  }
  return (
      (a.getAction() == b.getAction()) ? (a.getNextHopSet() < b.getNextHopSet())
                                       : a.getAction() < b.getAction());
}

// Methods for RouteNextHopEntry
void toAppend(const RouteNextHopEntry& entry, std::string* result) {
  result->append(entry.str());
}
std::ostream& operator<<(std::ostream& os, const RouteNextHopEntry& entry) {
  return os << entry.str();
}

// Methods for NextHopSet
void toAppend(const RouteNextHopEntry::NextHopSet& nhops, std::string* result) {
  for (const auto& nhop : nhops) {
    result->append(folly::to<std::string>(nhop.str(), " "));
  }
}

std::ostream& operator<<(
    std::ostream& os,
    const RouteNextHopEntry::NextHopSet& nhops) {
  for (const auto& nhop : nhops) {
    os << nhop.str() << " ";
  }
  return os;
}

NextHopWeight totalWeight(const RouteNextHopEntry::NextHopSet& nhops) {
  uint32_t result = 0;
  for (const auto& nh : nhops) {
    result += nh.weight();
  }
  return result;
}

folly::dynamic RouteNextHopEntry::toFollyDynamic() const {
  folly::dynamic entry = folly::dynamic::object;
  entry[kAction] = forwardActionStr(action_);
  folly::dynamic nhops = folly::dynamic::array;
  for (const auto& nhop : nhopSet_) {
    nhops.push_back(nhop.toFollyDynamic());
  }
  entry[kNexthops] = std::move(nhops);
  entry[kAdminDistance] = static_cast<int32_t>(adminDistance_);
  return entry;
}

RouteNextHopEntry RouteNextHopEntry::fromFollyDynamic(
    const folly::dynamic& entryJson) {
  Action action = str2ForwardAction(entryJson[kAction].asString());
  auto it = entryJson.find(kAdminDistance);
  AdminDistance adminDistance = (it == entryJson.items().end())
      ? AdminDistance::MAX_ADMIN_DISTANCE
      : AdminDistance(entryJson[kAdminDistance].asInt());
  RouteNextHopEntry entry(Action::DROP, adminDistance);
  entry.action_ = action;
  for (const auto& nhop : entryJson[kNexthops]) {
    entry.nhopSet_.insert(util::nextHopFromFollyDynamic(nhop));
  }
  return entry;
}

bool RouteNextHopEntry::isValid(bool forMplsRoute) const {
  bool valid = true;
  if (!forMplsRoute) {
    /* for ip2mpls routes, next hop label forwarding action must be push */
    for (const auto& nexthop : nhopSet_) {
      if (action_ != Action::NEXTHOPS) {
        continue;
      }
      if (nexthop.labelForwardingAction() &&
          nexthop.labelForwardingAction()->type() !=
              LabelForwardingAction::LabelForwardingType::PUSH) {
        return !valid;
      }
    }
  }
  return valid;
}

RouteNextHopEntry::NextHopSet RouteNextHopEntry::normalizedNextHops() const {
  NextHopSet normalizedNextHops;
  // 1)
  for (const auto& nhop : getNextHopSet()) {
    normalizedNextHops.insert(ResolvedNextHop(
        nhop.addr(),
        nhop.intf(),
        std::max(nhop.weight(), NextHopWeight(1)),
        nhop.labelForwardingAction()));
  }
  // 2)
  // Calculate the totalWeight. If that exceeds the max ecmp width, we use the
  // following heuristic algorithm:
  // 2a) Calculate the scaled factor FLAGS_ecmp_width/totalWeight.
  //     Without rounding, multiplying each weight by this will still yield
  //     correct weight ratios between the next hops.
  // 2b) Scale each next hop by the scaling factor, rounding down by default
  //     except for when weights go below 1. In that case, add them in as
  //     weight 1. At this point, we might _still_ be above FLAGS_ecmp_width,
  //     because we could have rounded too many 0s up to 1.
  // 2c) Do a final pass where we make up any remaining excess weight above
  //     FLAGS_ecmp_width by iteratively decrementing the max weight. If there
  //     are more than FLAGS_ecmp_width next hops, this cannot possibly succeed.
  NextHopWeight totalWeight = std::accumulate(
      normalizedNextHops.begin(),
      normalizedNextHops.end(),
      0,
      [](NextHopWeight w, const NextHop& nh) { return w + nh.weight(); });
  // Total weight after applying the scaling factor
  // FLAGS_ecmp_width/totalWeight to all next hops.
  NextHopWeight scaledTotalWeight = 0;
  if (totalWeight > FLAGS_ecmp_width) {
    XLOG(DBG2) << "Total weight of next hops exceeds max ecmp width: "
               << totalWeight << " > " << FLAGS_ecmp_width << " ("
               << normalizedNextHops << ")";
    // 2a)
    double factor = FLAGS_ecmp_width / static_cast<double>(totalWeight);
    NextHopSet scaledNextHops;
    // 2b)
    for (const auto& nhop : normalizedNextHops) {
      NextHopWeight w = std::max(
          static_cast<NextHopWeight>(nhop.weight() * factor), NextHopWeight(1));
      scaledNextHops.insert(ResolvedNextHop(
          nhop.addr(), nhop.intf(), w, nhop.labelForwardingAction()));
      scaledTotalWeight += w;
    }
    // 2c)
    if (scaledTotalWeight > FLAGS_ecmp_width) {
      XLOG(WARNING) << "Total weight of scaled next hops STILL exceeds max "
                    << "ecmp width: " << scaledTotalWeight << " > "
                    << FLAGS_ecmp_width << " (" << scaledNextHops << ")";
      // calculate number of times we need to decrement the max next hop
      NextHopWeight overflow = scaledTotalWeight - FLAGS_ecmp_width;
      for (int i = 0; i < overflow; ++i) {
        // find the max weight next hop
        auto maxItr = std::max_element(
            scaledNextHops.begin(),
            scaledNextHops.end(),
            [](const NextHop& n1, const NextHop& n2) {
              return n1.weight() < n2.weight();
            });
        XLOG(DBG2) << "Decrementing the weight of next hop: " << *maxItr;
        // create a clone of the max weight next hop with weight decremented
        ResolvedNextHop decMax = ResolvedNextHop(
            maxItr->addr(),
            maxItr->intf(),
            maxItr->weight() - 1,
            maxItr->labelForwardingAction());
        // remove the max weight next hop and replace with the
        // decremented version, if the decremented version would
        // not have weight 0. If it would have weight 0, that means
        // that we have > FLAGS_ecmp_width next hops.
        scaledNextHops.erase(maxItr);
        if (decMax.weight() > 0) {
          scaledNextHops.insert(decMax);
        }
      }
    }
    XLOG(DBG2) << "Scaled next hops from " << getNextHopSet() << " to "
               << scaledNextHops;
    normalizedNextHops = scaledNextHops;
  }
  return normalizedNextHops;
}

} // namespace facebook::fboss
