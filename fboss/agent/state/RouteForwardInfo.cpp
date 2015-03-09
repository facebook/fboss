/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "RouteForwardInfo.h"
#include <vector>

namespace {
constexpr auto kInterface = "interface";
constexpr auto kNexthop = "nexthop";
constexpr auto kNexthops = "nexthops";
constexpr auto kAction = "action";
}

namespace facebook { namespace fboss {

using std::string;
using std::vector;

// RouteForwardInfo::Nexthop class
std::string RouteForwardInfo::Nexthop::str() const {
  return folly::to<string>(nexthop, "@I", intf);
}

bool RouteForwardInfo::Nexthop::operator<(const Nexthop& fwd) const {
  if (intf < fwd.intf) {
    return true;
  } else if (intf > fwd.intf) {
    return false;
  } else {
    return nexthop < fwd.nexthop;
  }
}

bool RouteForwardInfo::Nexthop::operator==(const Nexthop& fwd) const {
  return intf == fwd.intf && nexthop == fwd.nexthop;
}

// RouteForwardInfo class
std::string RouteForwardInfo::str() const {
  std::string result;
  switch (action_) {
  case Action::DROP:
    result = "DROP";
    break;
  case Action::TO_CPU:
    result = "TO_CPU";
    break;
  case Action::NEXTHOPS:
    toAppend(getNexthops(), &result);
    break;
  default:
    CHECK(0);
    break;
  }
  return result;
}

folly::dynamic RouteForwardInfo::toFollyDynamic() const {
  folly::dynamic fwdInfo = folly::dynamic::object;
  fwdInfo[kAction] = forwardActionStr(action_);
  vector<folly::dynamic> nhops;
  for (const auto& nhop: nexthops_) {
    nhops.push_back(nhop.toFollyDynamic());
  }
  fwdInfo[kNexthops] = std::move(nhops);
  return fwdInfo;
}

RouteForwardInfo
RouteForwardInfo::fromFollyDynamic(const folly::dynamic& fwdInfoJson) {
  RouteForwardInfo fwdInfo;
  for (const auto& nhop: fwdInfoJson[kNexthops]) {
    fwdInfo.nexthops_.insert(Nexthop::fromFollyDynamic(nhop));
  }
  fwdInfo.action_ = str2ForwardAction(fwdInfoJson[kAction].asString());
  return fwdInfo;
}

folly::dynamic RouteForwardInfo::Nexthop::toFollyDynamic() const {
  folly::dynamic nhop = folly::dynamic::object;
  nhop[kInterface] = static_cast<uint32_t>(intf);
  nhop[kNexthop] = nexthop.str();
  return nhop;
}

RouteForwardInfo::Nexthop
RouteForwardInfo::Nexthop::fromFollyDynamic(const folly::dynamic& nhopJson) {
  return Nexthop(InterfaceID(nhopJson[kInterface].asInt()),
      folly::IPAddress(nhopJson[kNexthop].stringPiece()));
}

// Methods for RouteForwardInfo
void toAppend(const RouteForwardInfo& fwd, std::string *result) {
  result->append(fwd.str());
}
std::ostream& operator<<(std::ostream& os, const RouteForwardInfo& fwd) {
  return os << fwd.str();
}

// Methods for RouteForwardInfo::Nexthops
void toAppend(const RouteForwardNexthops& nhops, std::string *result) {
  for (const auto& nhop : nhops) {
    result->append(folly::to<std::string>(nhop.str(), " "));
  }
}
std::ostream& operator<<(std::ostream& os, const RouteForwardNexthops& nhops) {
  for (const auto& nhop : nhops) {
    os << nhop.str() << " ";
  }
  return os;
}

}}
