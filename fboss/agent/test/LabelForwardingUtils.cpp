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

} // namespace util
} // namespace fboss
} // namespace facebook
