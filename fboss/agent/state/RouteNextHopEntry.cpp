/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/state/RouteNextHopEntry.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/RouteNextHop.h"

#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <iterator>
#include <numeric>
#include "folly/IPAddress.h"

namespace {
constexpr auto kNexthops = "nexthops";
constexpr auto kAction = "action";
constexpr auto kAdminDistance = "adminDistance";
constexpr auto kCounterID = "counterID";
constexpr auto kClassID = "classID";
static constexpr int kMinSizeForWideEcmp{128};

std::vector<facebook::fboss::NextHopThrift> thriftNextHopsFromAddresses(
    const std::vector<facebook::network::thrift::BinaryAddress>& addrs) {
  std::vector<facebook::fboss::NextHopThrift> nhs;
  nhs.reserve(addrs.size());
  for (const auto& addr : addrs) {
    facebook::fboss::NextHopThrift nh;
    *nh.address() = addr;
    *nh.weight() = 0;
    nhs.emplace_back(std::move(nh));
  }
  return nhs;
}
} // namespace

DEFINE_bool(wide_ecmp, false, "Enable fixed width wide ECMP feature");
DEFINE_bool(optimized_ucmp, false, "Enable UCMP normalization optimizations");
DEFINE_double(ucmp_max_error, 0.05, "Max UCMP normalization error");

namespace facebook::fboss {

namespace util {

RouteNextHopSet toRouteNextHopSet(
    std::vector<NextHopThrift> const& nhs,
    bool allowV6NonLinkLocal) {
  RouteNextHopSet rnhs{};
  if (nhs.empty()) {
    return rnhs;
  }
  std::vector<NextHop> nexthops;
  rnhs.reserve(nhs.size());
  for (auto const& nh : nhs) {
    nexthops.emplace_back(fromThrift(nh, allowV6NonLinkLocal));
  }
  rnhs.insert(std::begin(nexthops), std::end(nexthops));
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

UnicastRoute toUnicastRoute(
    const folly::CIDRNetwork& nw,
    const RouteNextHopEntry& nhopEntry) {
  UnicastRoute thriftRoute;
  thriftRoute.dest()->ip() = network::toBinaryAddress(nw.first);
  thriftRoute.dest()->prefixLength() = nw.second;
  thriftRoute.action() = nhopEntry.getAction();
  if (nhopEntry.getCounterID().has_value()) {
    thriftRoute.counterID() = nhopEntry.getCounterID().value();
  }
  if (nhopEntry.getClassID().has_value()) {
    thriftRoute.classID() = nhopEntry.getClassID().value();
  }
  if (nhopEntry.getAction() == RouteForwardAction::NEXTHOPS) {
    thriftRoute.nextHops() = fromRouteNextHopSet(nhopEntry.getNextHopSet());
  }
  return thriftRoute;
}

} // namespace util

RouteNextHopEntry::RouteNextHopEntry(
    NextHopSet nhopSet,
    AdminDistance distance,
    std::optional<RouteCounterID> counterID,
    std::optional<AclLookupClass> classID) {
  if (nhopSet.size() == 0) {
    throw FbossError("Empty nexthop set is passed to the RouteNextHopEntry");
  }
  auto data = getRouteNextHopEntryThrift(
      Action::NEXTHOPS, distance, nhopSet, counterID, classID);
  this->fromThrift(std::move(data));
}

NextHopWeight RouteNextHopEntry::getTotalWeight() const {
  return totalWeight(getNextHopSet());
}

std::string RouteNextHopEntry::str_DEPRACATED() const {
  std::string result;
  switch (getAction()) {
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
  result += folly::to<std::string>(
      ";admin=", static_cast<int32_t>(getAdminDistance()));
  auto counterID = getCounterID();
  auto classID = getClassID();
  result += folly::to<std::string>(
      ";counterID=", counterID.has_value() ? *counterID : "none");
  result += folly::to<std::string>(
      ";classID=",
      classID.has_value()
          ? apache::thrift::util::enumNameSafe(AclLookupClass(*classID))
          : "none");
  return result;
}

std::string RouteNextHopEntry::str() const {
  std::string jsonStr;
  apache::thrift::SimpleJSONSerializer::serialize(this->toThrift(), &jsonStr);
  return jsonStr;
}

bool operator==(const RouteNextHopEntry& a, const RouteNextHopEntry& b) {
  return (
      a.getAction() == b.getAction() and
      a.getNextHopSet() == b.getNextHopSet() and
      a.getAdminDistance() == b.getAdminDistance() and
      a.getCounterID() == b.getCounterID() and
      a.getClassID() == b.getClassID());
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

bool RouteNextHopEntry::isValid(bool forMplsRoute) const {
  bool valid = true;
  if (!forMplsRoute) {
    /* for ip2mpls routes, next hop label forwarding action must be push */
    auto nhops = getNextHopSet();
    for (const auto& nexthop : *safe_cref<switch_state_tags::nexthops>()) {
      if (getAction() != Action::NEXTHOPS) {
        continue;
      }

      if (nexthop->safe_cref<common_if_tags::mplsAction>() &&
          *(nexthop->safe_cref<common_if_tags::mplsAction>()
                ->safe_cref<mpls_tags::strings::action>()) !=
              LabelForwardingAction::LabelForwardingType::PUSH) {
        return !valid;
      }
    }
  }
  return valid;
}

// A greedy algorithm to determine UCMP weight distribution when total
// weight is greater than ecmp_width.
// Input - [w(1), w(2), .., w(n)] represents weights of each path
// Output - [w'(1), w'(2), .., w'(n)]
// Constraints
// w'(i) is an Integer
// Sum[w'(i)] <= ecmp_width // Fits within ECMP Width, 64 or 128
// Minimize - Max(ErrorDeviation(i)) for i = 1 to n
// where ErrorDeviation(i) is percentage deviation of weight for index i
// from optimal allocation (without constraints)

//  a) Compute gcd and reduce weights by gcd
//  b) Calculate the scaled factor FLAGS_ecmp_width/totalWeight.
//     Without rounding, multiplying each weight by this will still yield
//     correct weight ratios between the next hops.
//  c) Scale each next hop by the scaling factor, rounding up.
//  d) We might have exceeded ecmp_width at this point due to rounding up.
//     while total weight > ecmp_width
//           1) Find entry with maximum deviation from ideal distribution
//           2) reduce the weight of max error entry
//  e) we have a potential solution. Continue searching for an optimal solution.
//     while total weight is not zero or tolernce not achieved
//           1) Find entry with maximum deviation from ideal distribution
//           2) reduce the weight of max error entry
//           3) If newly created set has lower max deviation, record this
//              as new optimal solution.

void RouteNextHopEntry::normalize(
    std::vector<NextHopWeight>& scaledWeights,
    NextHopWeight totalWeight) const {
  // This is the weight distribution without constraints
  std::vector<double> idealWeights;

  // compute normalization factor
  double factor = FLAGS_ecmp_width / static_cast<double>(totalWeight);
  NextHopWeight scaledTotalWeight = 0;
  for (auto& entry : scaledWeights) {
    // Compute the ideal distribution
    idealWeights.emplace_back(entry / static_cast<double>(totalWeight));
    // compute scaled weights by rounding up
    entry = static_cast<NextHopWeight>(std::ceil(entry * factor));
    scaledTotalWeight += entry;
  }

  // find entry with max error
  auto findMaxErrorEntry = [&idealWeights,
                            &scaledTotalWeight](const auto& scaledWeights) {
    double maxError = 0;
    int maxErrorIndex = 0;
    auto index = 0;
    for (auto entry : scaledWeights) {
      // percentage weight of total weight allocation for this member
      auto allocation = (double)entry / scaledTotalWeight;
      // measure of percentage weight deviation from ideal
      auto error = (allocation - idealWeights[index]) / idealWeights[index];
      // record the max error entry and it's key
      if (error >= maxError) {
        maxErrorIndex = index;
        maxError = error;
      }
      index++;
    }
    return std::tuple(maxErrorIndex, maxError);
  };

  // current solution is not feasible till it can fit in ecmp_width value
  while (scaledTotalWeight > FLAGS_ecmp_width) {
    auto key = std::get<0>(findMaxErrorEntry(scaledWeights));
    scaledWeights.at(key)--;
    scaledTotalWeight--;
  }

  // we have a potential solution now
  auto [maxErrorIndex, maxErrorVal] = findMaxErrorEntry(scaledWeights);
  auto bestMaxError = maxErrorVal;

  // Do not optimize further for wide ecmp
  bool wideEcmp = FLAGS_wide_ecmp && scaledTotalWeight > kMinSizeForWideEcmp;
  if (!wideEcmp) {
    // map to continue searching for best result
    auto newWeights = scaledWeights;

    while (scaledTotalWeight > 1 && bestMaxError > FLAGS_ucmp_max_error) {
      // reduce weight for entry with max error deviation
      newWeights.at(maxErrorIndex)--;
      scaledTotalWeight--;
      // recompute the new error deviation
      std::tie(maxErrorIndex, maxErrorVal) = findMaxErrorEntry(newWeights);
      // save new solution if it reduces max eror deviation
      if (maxErrorVal <= bestMaxError) {
        scaledWeights = newWeights;
        bestMaxError = maxErrorVal;
      }
    };
  }

  int commonFactor{0};
  // reduce weights proportionaly by gcd if needed
  for (const auto entry : scaledWeights) {
    if (entry) {
      commonFactor = std::gcd(commonFactor, entry);
    }
  }
  commonFactor = std::max(commonFactor, 1);

  // reduce weights by gcd if needed
  for (auto& entry : scaledWeights) {
    entry = entry / commonFactor;
  }
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
    XLOG(DBG3) << "Total weight of next hops exceeds max ecmp width: "
               << totalWeight << " > " << FLAGS_ecmp_width << " ("
               << normalizedNextHops << ")";

    NextHopSet scaledNextHops;
    if (FLAGS_optimized_ucmp) {
      std::vector<NextHopWeight> scaledWeights;
      for (const auto& nhop : normalizedNextHops) {
        scaledWeights.emplace_back(nhop.weight());
      }
      normalize(scaledWeights, totalWeight);

      auto index = 0;
      for (const auto& nhop : normalizedNextHops) {
        auto weight = scaledWeights.at(index);
        if (weight) {
          scaledNextHops.insert(ResolvedNextHop(
              nhop.addr(), nhop.intf(), weight, nhop.labelForwardingAction()));
          scaledTotalWeight += weight;
        }
        index++;
      }
    } else {
      // 2a)
      double factor = FLAGS_ecmp_width / static_cast<double>(totalWeight);
      // 2b)
      for (const auto& nhop : normalizedNextHops) {
        NextHopWeight w = std::max(
            static_cast<NextHopWeight>(nhop.weight() * factor),
            NextHopWeight(1));
        scaledNextHops.insert(ResolvedNextHop(
            nhop.addr(), nhop.intf(), w, nhop.labelForwardingAction()));
        scaledTotalWeight += w;
      }
      // 2c)
      if (scaledTotalWeight > FLAGS_ecmp_width) {
        XLOG(DBG3) << "Total weight of scaled next hops STILL exceeds max "
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
          XLOG(DBG3) << "Decrementing the weight of next hop: " << *maxItr;
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
        scaledTotalWeight = FLAGS_ecmp_width;
      }
    }
    XLOG(DBG3) << "Scaled next hops from " << getNextHopSet() << " to "
               << scaledNextHops;
    normalizedNextHops = scaledNextHops;
  } else {
    scaledTotalWeight = totalWeight;
  }

  if (FLAGS_wide_ecmp && scaledTotalWeight > kMinSizeForWideEcmp &&
      scaledTotalWeight < FLAGS_ecmp_width) {
    std::vector<uint64_t> nhopWeights;
    for (const auto& nhop : normalizedNextHops) {
      nhopWeights.emplace_back(nhop.weight());
    }
    normalizeNextHopWeightsToMaxPaths(nhopWeights, FLAGS_ecmp_width);
    NextHopSet normalizedToMaxPathNextHops;
    int idx = 0;
    for (const auto& nhop : normalizedNextHops) {
      normalizedToMaxPathNextHops.insert(ResolvedNextHop(
          nhop.addr(),
          nhop.intf(),
          nhopWeights.at(idx++),
          nhop.labelForwardingAction()));
    }
    XLOG(DBG3) << "Scaled next hops from " << getNextHopSet() << " to "
               << normalizedToMaxPathNextHops;
    return normalizedToMaxPathNextHops;
  }
  return normalizedNextHops;
}

RouteNextHopEntry RouteNextHopEntry::from(
    const facebook::fboss::UnicastRoute& route,
    AdminDistance defaultAdminDistance,
    std::optional<RouteCounterID> counterID,
    std::optional<AclLookupClass> classID) {
  std::vector<NextHopThrift> nhts;
  if (route.nextHops()->empty() && !route.nextHopAddrs()->empty()) {
    nhts = ::thriftNextHopsFromAddresses(*route.nextHopAddrs());
  } else {
    nhts = *route.nextHops();
  }

  RouteNextHopSet nexthops = util::toRouteNextHopSet(nhts);

  auto adminDistance = route.adminDistance().value_or(defaultAdminDistance);

  if (nexthops.size()) {
    if (route.action() &&
        *route.action() != facebook::fboss::RouteForwardAction::NEXTHOPS) {
      throw FbossError(
          "Nexthops specified, but action is set to : ", *route.action());
    }
    return RouteNextHopEntry(
        std::move(nexthops), adminDistance, counterID, classID);
  }

  if (!route.action() ||
      *route.action() == facebook::fboss::RouteForwardAction::DROP) {
    return RouteNextHopEntry(
        RouteForwardAction::DROP, adminDistance, counterID);
  }
  return RouteNextHopEntry(
      RouteForwardAction::TO_CPU, adminDistance, counterID);
}

RouteNextHopEntry RouteNextHopEntry::from(
    const facebook::fboss::MplsRoute& route,
    AdminDistance defaultAdminDistance,
    std::optional<RouteCounterID> counterID,
    std::optional<AclLookupClass> classID) {
  RouteNextHopSet nexthops = util::toRouteNextHopSet(*route.nextHops());
  auto adminDistance = route.adminDistance().value_or(defaultAdminDistance);
  if (nexthops.size()) {
    return {std::move(nexthops), adminDistance, counterID, classID};
  }
  return {RouteForwardAction::TO_CPU, adminDistance, counterID, classID};
}

RouteNextHopEntry RouteNextHopEntry::createDrop(AdminDistance adminDistance) {
  return RouteNextHopEntry(RouteForwardAction::DROP, adminDistance);
}

RouteNextHopEntry RouteNextHopEntry::createToCpu(AdminDistance adminDistance) {
  return RouteNextHopEntry(RouteForwardAction::TO_CPU, adminDistance);
}

RouteNextHopEntry RouteNextHopEntry::fromStaticRoute(
    const cfg::StaticRouteWithNextHops& route) {
  RouteNextHopSet nhops;

  // NOTE: Static routes use the default UCMP weight so that they can be
  // compatible with UCMP, i.e., so that we can do ucmp where the next
  // hops resolve to a static route.  If we define recursive static
  // routes, that may lead to unexpected behavior where some interface
  // gets more traffic.  If necessary, in the future, we can make it
  // possible to configure strictly ECMP static routes
  for (auto& nhopStr : *route.nexthops()) {
    nhops.emplace(
        UnresolvedNextHop(folly::IPAddress(nhopStr), UCMP_DEFAULT_WEIGHT));
  }

  return RouteNextHopEntry(std::move(nhops), AdminDistance::STATIC_ROUTE);
}

RouteNextHopEntry RouteNextHopEntry::fromStaticIp2MplsRoute(
    const cfg::StaticIp2MplsRoute& route) {
  RouteNextHopSet nhops;

  for (auto& nexthop : *route.nexthops()) {
    nhops.emplace(util::fromThrift(nexthop));
  }
  return RouteNextHopEntry(std::move(nhops), AdminDistance::STATIC_ROUTE);
}

RouteNextHopEntry RouteNextHopEntry::fromStaticMplsRoute(
    const cfg::StaticMplsRouteWithNextHops& route) {
  RouteNextHopSet nhops;

  for (auto& nexthop : *route.nexthops()) {
    nhops.emplace(util::fromThrift(nexthop));
  }
  return RouteNextHopEntry(std::move(nhops), AdminDistance::STATIC_ROUTE);
}

bool RouteNextHopEntry::isUcmp(const NextHopSet& nhopSet) {
  return totalWeight(nhopSet) != nhopSet.size();
}

/*
 * utility to normalize weights to a fixed total count
 */

// Modify ucmp weight distribution such that total weight is equal to
// a fixed value (normalizedPathCount). This is needed to support
// wide ucmp on TH3 where the total member count in ECMP structure
// programmed in hardware has be a power of 2. A high level description
// of normalization steps with an example walk through is included below.
//
// Consider a UCMP with following weight distribution
//      Number of nexthops      Weight
//          20                    8
//          10                    5
//          15                    7
//          10                    2
//
// 1. Compute a scale factor for weight so that weights can be adjusted
//    to a number close to normalized count.
//         factor = normalized count / total weight
//    In this example factor = 512/335 = 1.528
// 2. Scale weights by the factor computed
//    In the example, the scaled weights are
//      Number of nexthops      Weight
//          20                   12
//          10                    7
//          15                   10
//          10                    3
//    total scaled weight = 490
// 3. Compute underflow = normalized count - total weight.
//    In this example underflow = 512 - 490 = 22
// 4. Allocate the underflow slots to the highest weight next
//    hop sets that can accommodate the slots. This is done
//    with the intention of reducing the percent traffic load
//    difference between members of same weight group to minimum.
//    In this example, the largest 2 weights (12 and 10) has total
//    20 + 15 = 35 nexthops. Allocate 22 remaining slots among
//    these 2 weight groups by filling the lowest weight group first.
//    Weight 10 will get 15 slots (weight becomes 11 now) and
//    weight 12 will get 7 nexthops (split to 2 groups of weight 12 and 13)
//      Number of nexthops      Weight
//           7                   13
//          13                   12
//          10                    7
//          15                   11
//          10                    3
// Total count = 512

void RouteNextHopEntry::normalizeNextHopWeightsToMaxPaths(
    std::vector<uint64_t>& nhWeights,
    uint64_t normalizedPathCount) {
  uint64_t totalWeight = 0;
  for (const auto weight : nhWeights) {
    totalWeight += weight;
  }
  // compute the scale factor
  double factor = normalizedPathCount / static_cast<double>(totalWeight);
  uint64_t scaledTotalWeight = 0;
  std::map<uint64_t, uint32_t> WeightMap;

  for (auto& weight : nhWeights) {
    weight = std::max(int(weight * factor), 1);
    WeightMap[weight]++;
    scaledTotalWeight += weight;
  }
  if (scaledTotalWeight < normalizedPathCount) {
    int underflow = normalizedPathCount - scaledTotalWeight;

    std::map<int, int> underflowWeightAllocation;
    // distribute any remaining underflows to most weighted nhops
    int underflowAllocated = 0;
    // check from highest weight to lowest
    for (auto it = WeightMap.rbegin(); it != WeightMap.rend(); it++) {
      underflowAllocated += it->second;
      underflowWeightAllocation[it->first] = it->second;
      // stop when we find enough nexthops to absord underflow
      if (underflowAllocated >= underflow) {
        break;
      }
    }

    uint32_t overAllocation = underflowAllocated - underflow;
    // Shift uneven allocation to the highest weight groups
    // so that percentage error between members of same group is minimum.
    for (auto it = WeightMap.rbegin(); it != WeightMap.rend(); it++) {
      int delta = std::min(overAllocation, it->second);
      underflowWeightAllocation[it->first] -= delta;
      overAllocation -= delta;
      if (!overAllocation) {
        break;
      }
    }

    // distribute the underflow evenly across the members in weight groups
    for (auto& weight : nhWeights) {
      if (underflow && underflowWeightAllocation[weight]) {
        underflowWeightAllocation[weight]--;
        weight++;
        underflow--;
      }
    }
    // we should allocate all underflow slots
    CHECK_EQ(underflow, 0);
  }
}

state::RouteNextHopEntry RouteNextHopEntry::getRouteNextHopEntryThrift(
    Action action,
    AdminDistance distance,
    NextHopSet nhopSet,
    std::optional<RouteCounterID> counterID,
    std::optional<AclLookupClass> classID) {
  state::RouteNextHopEntry entry{};
  entry.adminDistance() = distance;
  entry.action() = action;
  if (counterID) {
    entry.counterID() = *counterID;
  }
  if (classID) {
    entry.classID() = *classID;
  }
  if (!nhopSet.empty()) {
    entry.nexthops() = util::fromRouteNextHopSet(std::move(nhopSet));
  }
  return entry;
}

// THRIFT_COPY
RouteNextHopSet RouteNextHopEntry::getNextHopSet() const {
  return util::toRouteNextHopSet(
      safe_cref<switch_state_tags::nexthops>()->toThrift(), true);
}

} // namespace facebook::fboss
