// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/String.h>
#include <folly/init/Init.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <unistd.h>
#include <csignal>

#include "fboss/fsdb/common/Flags.h"
#include "fboss/fsdb/tests/utils/bgp_rib_test_publisher/BgpRibTestPublisher.h"

DEFINE_string(
    routes,
    "",
    "Comma-separated list of prefixes to publish (e.g., 2001:db8:1::/48,10.0.0.0/24)");
DEFINE_string(
    next_hops,
    "",
    "Comma-separated list of next hops, 1:1 with --routes (e.g., 2001:db8:ffff::1,10.255.0.1)");

using namespace facebook::fboss::fsdb::test;

class SignalHandler : public folly::AsyncSignalHandler {
 public:
  SignalHandler(
      folly::EventBase* evb,
      BgpRibTestPublisher& publisher,
      const std::map<std::string, bgp_thrift::TRibEntry>& routes)
      : folly::AsyncSignalHandler(evb),
        evb_(evb),
        publisher_(publisher),
        routesToPublish_(routes) {}

  void signalReceived(int signum) noexcept override {
    if (signum == SIGUSR1) {
      XLOG(INFO) << "Received SIGUSR1: withdrawing routes and exiting";
      publisher_.withdrawAllRoutes();
      evb_->terminateLoopSoon();
    } else if (signum == SIGUSR2) {
      XLOG(INFO) << "Received SIGUSR2: withdrawing routes, keeping running";
      publisher_.withdrawAllRoutes();
    } else if (signum == SIGHUP) {
      XLOG(INFO) << "Received SIGHUP: republishing routes";
      publisher_.publishRibMap(routesToPublish_);
      XLOG(INFO) << "Republished " << routesToPublish_.size() << " routes";
    } else if (signum == SIGINT || signum == SIGTERM) {
      XLOG(INFO) << "Received SIGINT/SIGTERM: graceful shutdown";
      evb_->terminateLoopSoon();
    }
  }

 private:
  folly::EventBase* evb_;
  BgpRibTestPublisher& publisher_;
  const std::map<std::string, bgp_thrift::TRibEntry>& routesToPublish_;
};

int main(int argc, char** argv) {
  FLAGS_publish_state_to_fsdb = true;

  folly::Init init(&argc, &argv);

  CHECK(!FLAGS_routes.empty()) << "--routes is required";
  CHECK(!FLAGS_next_hops.empty()) << "--next-hops is required";

  std::vector<std::string> prefixes;
  folly::split(',', FLAGS_routes, prefixes, true /* ignoreEmpty */);
  std::vector<std::string> nextHops;
  folly::split(',', FLAGS_next_hops, nextHops, true /* ignoreEmpty */);
  CHECK_EQ(prefixes.size(), nextHops.size())
      << "--routes and --next-hops must have the same number of entries ("
      << prefixes.size() << " vs " << nextHops.size() << ")";

  BgpRibTestPublisher publisher("bgp-rib-test-publisher");
  publisher.start();

  std::map<std::string, bgp_thrift::TRibEntry> routesToPublish;
  for (size_t i = 0; i < prefixes.size(); ++i) {
    auto entry =
        BgpRibTestPublisher::createTestRibEntry(prefixes[i], nextHops[i]);
    routesToPublish[prefixes[i]] = entry;
    XLOG(INFO) << "Route: " << prefixes[i] << " via " << nextHops[i];
  }
  publisher.publishRibMap(routesToPublish);
  XLOG(INFO) << "Published " << routesToPublish.size() << " routes";

  folly::EventBase evb;

  SignalHandler sigHandler(&evb, publisher, routesToPublish);
  sigHandler.registerSignalHandler(SIGUSR1);
  sigHandler.registerSignalHandler(SIGUSR2);
  sigHandler.registerSignalHandler(SIGHUP);
  sigHandler.registerSignalHandler(SIGINT);
  sigHandler.registerSignalHandler(SIGTERM);

  XLOG(INFO) << "";
  XLOG(INFO) << "========================================";
  XLOG(INFO) << "Publisher running. PID: " << getpid();
  XLOG(INFO) << "========================================";
  XLOG(INFO) << "Signal handlers:";
  XLOG(INFO) << "  SIGUSR1 (kill -USR1 " << getpid()
             << ") - Withdraw routes and exit";
  XLOG(INFO) << "  SIGUSR2 (kill -USR2 " << getpid()
             << ") - Withdraw routes, keep running";
  XLOG(INFO) << "  SIGHUP  (kill -HUP " << getpid() << ") - Republish routes";
  XLOG(INFO) << "  SIGINT/SIGTERM - Graceful shutdown (no withdrawal)";
  XLOG(INFO) << "========================================";
  XLOG(INFO) << "";

  evb.loopForever();

  publisher.stop();
  XLOG(INFO) << "Publisher stopped";
  return 0;
}
