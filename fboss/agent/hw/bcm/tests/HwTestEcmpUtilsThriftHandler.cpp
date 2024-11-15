// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/test/HwTestThriftHandler.h"

#include "fboss/agent/hw/test/HwTestEcmpUtils.h"

namespace facebook {
namespace fboss {
namespace utility {

int HwTestThriftHandler::getHwEcmpSize(
    std::unique_ptr<CIDRNetwork> prefix,
    int routerID,
    int sizeInSw) {
  folly::CIDRNetwork ecmpPrefix = std::make_pair(
      folly::IPAddress(prefix->IPAddress_ref().value()),
      prefix->mask_ref().value());
  facebook::fboss::RouterID rid =
      static_cast<facebook::fboss::RouterID>(routerID);
  return getEcmpMembersInHw(hwSwitch_, ecmpPrefix, rid, sizeInSw).size();
}

void HwTestThriftHandler::getEcmpWeights(
    std::map<::std::int32_t, ::std::int32_t>& weights,
    std::unique_ptr<CIDRNetwork> prefix,
    int routerID) {
  folly::CIDRNetwork ecmpPrefix = std::make_pair(
      folly::IPAddress(prefix->IPAddress_ref().value()),
      prefix->mask_ref().value());
  facebook::fboss::RouterID rid =
      static_cast<facebook::fboss::RouterID>(routerID);
  auto members =
      getEcmpMembersInHw(hwSwitch_, ecmpPrefix, rid, FLAGS_ecmp_width);
  std::set<uint64_t> uniqueMembers(members.begin(), members.end());
  int index = 0;
  for (auto member : uniqueMembers) {
    weights.insert(
        std::make_pair(index, getEcmpMemberWeight(hwSwitch_, members, member)));
    index++;
  }
  return;
}

} // namespace utility
} // namespace fboss
} // namespace facebook
