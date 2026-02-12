// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/init/Init.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <unistd.h>
#include <csignal>

#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/tests/utils/bgp_rib_test_publisher/BgpRibTestPublisher.h"

DEFINE_int32(num_routes, 10, "Number of synthetic routes to generate");
DEFINE_string(prefix_base, "2001:db8:", "Base prefix for synthetic routes");
DEFINE_string(
    nexthop_base,
    "2001:db8:ffff::",
    "Base nexthop for synthetic routes");

using namespace facebook::fboss::fsdb::test;

int main(int argc, char** argv) {
  FLAGS_publish_state_to_fsdb = true;

  folly::Init init(&argc, &argv);

  BgpRibTestPublisher publisher("bgp-rib-test-publisher");
  publisher.start();

  auto ribMap = BgpRibTestPublisher::createTestRibMap(
      FLAGS_num_routes, FLAGS_prefix_base, FLAGS_nexthop_base);
  publisher.publishRibMap(ribMap);
  XLOG(INFO) << "Published " << FLAGS_num_routes << " routes";

  XLOG(INFO) << "Publisher running. PID: " << getpid();
  XLOG(INFO) << "Send SIGINT/SIGTERM to stop.";

  folly::EventBase evb;
  folly::CallbackAsyncSignalHandler cbHandler(
      &evb, [&evb](int) { evb.terminateLoopSoon(); });
  cbHandler.registerSignalHandler(SIGINT);
  cbHandler.registerSignalHandler(SIGTERM);

  evb.loopForever();

  publisher.stop();
  XLOG(INFO) << "Publisher stopped";
  return 0;
}
