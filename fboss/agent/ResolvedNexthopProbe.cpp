// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/ResolvedNexthopProbe.h"
#include "fboss/agent/SwSwitch.h"
#include "folly/Random.h"

#include <chrono>

namespace {
const std::chrono::milliseconds kInitialBackoff{1000};
const std::chrono::milliseconds kMaximumBackoff{10000};
auto constexpr kJitterPct = 10;
} // namespace

namespace facebook {
namespace fboss {

ResolvedNextHopProbe::ResolvedNextHopProbe(
    folly::EventBase* evb,
    ResolvedNextHop nexthop)
    : folly::AsyncTimeout(evb),
      evb_(evb),
      nexthop_(nexthop),
      backoff_(kInitialBackoff, kMaximumBackoff) {}

void ResolvedNextHopProbe::timeoutExpired() noexcept {
  auto ip = nexthop_.addr();
  if (ip.isV4()) {
    // send arp request
  } else {
    // send ndp request
  }
  // exponential back-off
  backoff_.reportError();
  // add jitter to reduce contention
  auto backoff = backoff_.getTimeRemainingUntilRetry();
  auto timeout = backoff.count() +
      (folly::Random::rand32() % (backoff.count() * kJitterPct / 100));
  scheduleTimeout(timeout);
}
} // namespace fboss
} // namespace facebook
