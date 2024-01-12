// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/InterfaceStats.h"

#include <fb303/ThreadCachedServiceData.h>
#include <folly/String.h>
#include "fboss/agent/SwitchStats.h"

using facebook::fb303::SUM;

namespace {
const std::string kNameKeySeperator = ".";
const std::string kRouterAdvertisement = "router_advertisements";

} // namespace
namespace facebook::fboss {

InterfaceStats::InterfaceStats(
    InterfaceID intfID,
    std::string intfName,
    SwitchStats* switchStats)
    : intfID_(intfID), intfName_(intfName), switchStats_(switchStats) {
  if (!intfName_.empty()) {
    tcData().addStatValue(getCounterKey(kRouterAdvertisement), 0, SUM);
  }
}

InterfaceStats::~InterfaceStats() {
  // clear counter
  clearCounters();
}

void InterfaceStats::sentRouterAdvertisement() {
  if (!intfName_.empty()) {
    tcData().addStatValue(getCounterKey(kRouterAdvertisement), 1, SUM);
  }
}

void InterfaceStats::clearCounters() {
  tcData().clearCounter(getCounterKey(kRouterAdvertisement));
}

std::string InterfaceStats::getCounterKey(const std::string& key) {
  return folly::to<std::string>(intfName_, kNameKeySeperator, key);
}
} // namespace facebook::fboss
