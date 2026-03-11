// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/init/Init.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <unistd.h>
#include <csignal>

#include "fboss/fsdb/tests/utils/bgp_route_injector/RouteInjector.h"

DEFINE_string(
    routes_file,
    "",
    "Path to file containing routes. Re-read on SIGHUP for runtime updates.");
DEFINE_string(agent_host, "127.0.0.1", "FBOSS agent host");
DEFINE_int32(agent_port, 5909, "FBOSS agent Thrift port");

using namespace facebook::fboss;

namespace {

std::vector<RouteSpec> parseRouteLine(const std::string& line) {
  std::vector<RouteSpec> specs;
  std::vector<std::string> routeParts;
  folly::split(';', line, routeParts, true);

  for (const auto& routePart : routeParts) {
    auto eqPos = routePart.find('=');
    if (eqPos == std::string::npos) {
      XLOG(ERR) << "Invalid route format: " << routePart
                << " (expected prefix=nh1,nh2)";
      continue;
    }

    RouteSpec spec;
    spec.prefix = routePart.substr(0, eqPos);
    auto nhStr = routePart.substr(eqPos + 1);

    std::vector<std::string> nextHops;
    folly::split(',', nhStr, nextHops, true);

    if (nextHops.empty()) {
      XLOG(ERR) << "No next-hops specified for prefix: " << spec.prefix;
      continue;
    }

    spec.nextHops = std::move(nextHops);
    specs.push_back(std::move(spec));
  }
  return specs;
}

std::vector<RouteSpec> loadRoutesFromFile(const std::string& filePath) {
  std::string content;
  if (!folly::readFile(filePath.c_str(), content)) {
    XLOG(ERR) << "Failed to read routes file: " << filePath;
    return {};
  }

  std::vector<std::string> lines;
  folly::split('\n', content, lines, true);

  std::vector<RouteSpec> allSpecs;
  for (const auto& line : lines) {
    auto trimmed = folly::trimWhitespace(line).str();
    if (trimmed.empty() || trimmed[0] == '#') {
      continue;
    }
    auto specs = parseRouteLine(trimmed);
    allSpecs.insert(allSpecs.end(), specs.begin(), specs.end());
  }
  return allSpecs;
}

void printRoutes(const std::vector<RouteSpec>& specs) {
  for (const auto& spec : specs) {
    std::string nhList;
    for (size_t i = 0; i < spec.nextHops.size(); ++i) {
      if (i > 0) {
        nhList += ", ";
      }
      nhList += spec.nextHops[i];
    }
    XLOG(INFO) << "  " << spec.prefix << " via [" << nhList << "]";
  }
}

} // namespace

class SignalHandler : public folly::AsyncSignalHandler {
 public:
  SignalHandler(folly::EventBase* evb, RouteInjector& injector)
      : folly::AsyncSignalHandler(evb), evb_(evb), injector_(injector) {}

  void signalReceived(int signum) noexcept override {
    if (signum == SIGUSR1) {
      XLOG(INFO) << "SIGUSR1: deleting routes and exiting";
      injector_.deleteAllRoutes();
      evb_->terminateLoopSoon();
    } else if (signum == SIGUSR2) {
      XLOG(INFO) << "SIGUSR2: deleting routes, keeping running";
      injector_.deleteAllRoutes();
    } else if (signum == SIGHUP) {
      XLOG(INFO) << "SIGHUP: reloading routes from " << FLAGS_routes_file;
      auto routeSpecs = loadRoutesFromFile(FLAGS_routes_file);
      if (!routeSpecs.empty()) {
        injector_.deleteAllRoutes();
        injector_.addRoutes(routeSpecs);
        XLOG(INFO) << "Updated with " << routeSpecs.size() << " routes:";
        printRoutes(routeSpecs);
      } else {
        XLOG(WARN) << "No routes loaded, keeping existing routes";
      }
    } else if (signum == SIGINT || signum == SIGTERM) {
      XLOG(INFO) << "SIGINT/SIGTERM: graceful shutdown";
      evb_->terminateLoopSoon();
    }
  }

 private:
  folly::EventBase* evb_;
  RouteInjector& injector_;
};

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);

  CHECK(!FLAGS_routes_file.empty()) << "--routes_file is required";

  auto routeSpecs = loadRoutesFromFile(FLAGS_routes_file);
  CHECK(!routeSpecs.empty()) << "No valid route specifications found";

  XLOG(INFO) << "========================================";
  XLOG(INFO) << "Route Injector (FBOSS Agent API)";
  XLOG(INFO) << "========================================";
  XLOG(INFO) << "Agent: " << FLAGS_agent_host << ":" << FLAGS_agent_port;
  XLOG(INFO) << "Routes file: " << FLAGS_routes_file;
  XLOG(INFO) << "Routes to inject:";
  printRoutes(routeSpecs);
  XLOG(INFO) << "========================================";

  RouteInjector injector(FLAGS_agent_host, FLAGS_agent_port);
  injector.addRoutes(routeSpecs);
  XLOG(INFO) << "Injected " << routeSpecs.size() << " routes";

  folly::EventBase evb;
  SignalHandler sigHandler(&evb, injector);
  sigHandler.registerSignalHandler(SIGUSR1);
  sigHandler.registerSignalHandler(SIGUSR2);
  sigHandler.registerSignalHandler(SIGHUP);
  sigHandler.registerSignalHandler(SIGINT);
  sigHandler.registerSignalHandler(SIGTERM);

  XLOG(INFO) << "Injector running. PID: " << getpid();
  XLOG(INFO) << "  SIGUSR1 - Delete routes and exit";
  XLOG(INFO) << "  SIGUSR2 - Delete routes, keep running";
  XLOG(INFO) << "  SIGHUP  - Reload routes from file";
  XLOG(INFO) << "  SIGINT/SIGTERM - Graceful shutdown";

  evb.loopForever();
  XLOG(INFO) << "Injector stopped";
  return 0;
}
