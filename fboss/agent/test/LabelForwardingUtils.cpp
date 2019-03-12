// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/test/LabelForwardingUtils.h"

namespace {
using namespace facebook::fboss;

const std::array<folly::IPAddress, 2> kNextHopAddrs{
    folly::IPAddress("10.0.0.1"),
    folly::IPAddress("10.0.0.2"),
};
const std::array<LabelForwardingAction::LabelStack, 2> kLabelStacks{
    LabelForwardingAction::LabelStack{1001, 1002},
    LabelForwardingAction::LabelStack{2001, 2002}};
} // namespace

namespace facebook {
namespace fboss {
namespace util {

LabelForwardingAction getSwapAction(LabelForwardingAction::Label swapWith) {
  return LabelForwardingAction(
      LabelForwardingAction::LabelForwardingType::SWAP, swapWith);
}

LabelForwardingAction getPhpAction(bool isPhp) {
  return LabelForwardingAction(
      isPhp ? LabelForwardingAction::LabelForwardingType::PHP
            : LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
}

LabelForwardingAction getPushAction(LabelForwardingAction::LabelStack stack) {
  return LabelForwardingAction(
      LabelForwardingAction::LabelForwardingType::PUSH, std::move(stack));
}

LabelNextHopEntry getSwapLabelNextHopEntry(AdminDistance distance) {
  LabelNextHopSet nexthops;
  for (auto i = 0; i < kNextHopAddrs.size(); i++) {
    nexthops.emplace(ResolvedNextHop(
        kNextHopAddrs[i],
        InterfaceID(i + 1),
        ECMP_WEIGHT,
        getSwapAction(kLabelStacks[i][0])));
  }
  return LabelNextHopEntry(std::move(nexthops), distance);
}

LabelNextHopEntry getPushLabelNextHopEntry(AdminDistance distance) {
  LabelNextHopSet nexthops;
  for (auto i = 0; i < kNextHopAddrs.size(); i++) {
    nexthops.emplace(ResolvedNextHop(
        kNextHopAddrs[i],
        InterfaceID(i + 1),
        ECMP_WEIGHT,
        getPushAction(kLabelStacks[i])));
  }
  return LabelNextHopEntry(std::move(nexthops), distance);
}

LabelNextHopEntry getPhpLabelNextHopEntry(AdminDistance distance) {
  LabelNextHopSet nexthops;
  for (auto i = 0; i < kNextHopAddrs.size(); i++) {
    nexthops.emplace(ResolvedNextHop(
        kNextHopAddrs[i], InterfaceID(i + 1), ECMP_WEIGHT, getPhpAction()));
  }
  return LabelNextHopEntry(std::move(nexthops), distance);
}

LabelNextHopEntry getPopLabelNextHopEntry(AdminDistance distance) {
  LabelNextHopSet nexthops;
  for (auto i = 0; i < kNextHopAddrs.size(); i++) {
    nexthops.emplace(ResolvedNextHop(
        kNextHopAddrs[i],
        InterfaceID(i + 1),
        ECMP_WEIGHT,
        getPhpAction(false)));
  }
  return LabelNextHopEntry(std::move(nexthops), distance);
}

NextHopThrift getSwapNextHopThrift(int offset) {
  auto nexthopIp = folly::IPAddressV6::tryFromString(
      folly::to<std::string>("fe80::", offset));
  NextHopThrift nexthop;
  nexthop.address.addr.append(
      reinterpret_cast<const char*>(nexthopIp->bytes()),
      folly::IPAddressV6::byteCount());
  nexthop.address.ifName_ref() = folly::to<std::string>("fboss0", offset);
  MplsAction action;
  action.action = MplsActionCode::SWAP;
  action.swapLabel_ref() = 601;
  return nexthop;
}

MplsRoute getMplsRoute(MplsLabel label, AdminDistance distance) {
  MplsRoute route;
  route.topLabel = label;
  route.adminDistance_ref() = distance;
  for (auto i = 1; i < 5; i++) {
    route.nextHops.emplace_back(getSwapNextHopThrift(i));
  }
  return route;
}

std::vector<MplsRoute> getTestRoutes(int base, int count) {
  // TODO - put this in resource generator
  std::vector<MplsRoute> routes;
  for (auto i = base; i < base + count; i++) {
    routes.emplace_back(
        getMplsRoute(501 + i, AdminDistance::DIRECTLY_CONNECTED));
  }
  return routes;
}

} // namespace util
} // namespace fboss
} // namespace facebook
