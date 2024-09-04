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

} // namespace utility
} // namespace fboss
} // namespace facebook
