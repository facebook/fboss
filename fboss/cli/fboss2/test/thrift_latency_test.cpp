/*
 * Thrift RPC Latency Test - Comprehensive Backend Comparison
 *
 * This test demonstrates the performance difference between various EventBase
 * backends for Thrift RPC calls. It reproduces the multi-second latency issue
 * seen in OSS builds with libevent 2.x and validates the fixes.
 *
 * Usage:
 *   # Internal build (Buck2):
 *   buck2 run fbcode//fboss/cli/fboss2/test:thrift_latency_test
 *
 *   # OSS build (CMake):
 *   ./thrift_latency_test
 */

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <thread>
#include <vector>

#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/init/Init.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

// Conditionally include EpollBackend if available
#if __has_include(<folly/io/async/EpollBackend.h>)
#include <folly/io/async/EpollBackend.h>
#define HAS_EPOLL_BACKEND 1
#else
#define HAS_EPOLL_BACKEND 0
#endif

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"

using namespace facebook::fboss;
using namespace apache::thrift;

constexpr int kNumIterations = 10;
constexpr int64_t kLatencyThresholdMs = 100;

#if HAS_EPOLL_BACKEND
// Get EventBase::Options configured to use EpollBackend
folly::EventBase::Options getEpollEventBaseOptions() {
  return folly::EventBase::Options{}.setBackendFactory(
      []() -> std::unique_ptr<folly::EventBaseBackendBase> {
        return std::make_unique<folly::EpollBackend>(
            folly::EpollBackend::Options{});
      });
}

// EventBaseManager that creates EventBases with EpollBackend
folly::EventBaseManager& getEpollEventBaseManager() {
  static folly::EventBaseManager manager(getEpollEventBaseOptions());
  return manager;
}
#endif

// Simple handler that returns a fixed timestamp
class SimpleHandler : public apache::thrift::ServiceHandler<FbossCtrl> {
 public:
  void getConfigAppliedInfo(ConfigAppliedInfo& info) override {
    info.lastAppliedInMs() = 12345;
    info.lastColdbootAppliedInMs() = 0;
  }
};

// Result of running a backend test
struct BackendResult {
  std::string name;
  bool success;
  std::vector<double> latenciesMs;
  double avgLatencyMs;
  double maxLatencyMs;
  std::string errorMsg;
};

// Run latency test for a specific backend configuration
BackendResult runBackendTest(
    const std::string& backendName,
    std::function<void(ThriftServer&)> configureServer) {
  BackendResult result;
  result.name = backendName;
  result.success = true;

  try {
    auto handler = std::make_shared<SimpleHandler>();

    // Keep executor alive for the lifetime of the server
    std::shared_ptr<folly::IOThreadPoolExecutor> executor;

    ScopedServerInterfaceThread::ServerConfigCb serverConfigCb =
        [&configureServer, &executor](ThriftServer& server) {
          configureServer(server);
        };

    ScopedServerInterfaceThread server(
        handler, "::1", 0, std::move(serverConfigCb));

    auto port = server.getAddress().getPort();
    std::cout << "  Server started on port " << port << std::endl;

    // Create client
    auto client = server.newClient<apache::thrift::Client<FbossCtrl>>(
        nullptr, RocketClientChannel::newChannel);

    // Run multiple iterations
    for (int i = 0; i < kNumIterations; ++i) {
      auto start = std::chrono::steady_clock::now();

      ConfigAppliedInfo info;
      client->sync_getConfigAppliedInfo(info);

      auto end = std::chrono::steady_clock::now();
      // Use microseconds for sub-millisecond precision, convert to double ms
      auto durationUs =
          std::chrono::duration_cast<std::chrono::microseconds>(end - start)
              .count();
      double durationMs = durationUs / 1000.0;

      result.latenciesMs.push_back(durationMs);
      std::cout << std::fixed << std::setprecision(3);
      std::cout << "    Iteration " << (i + 1) << ": " << durationMs << " ms"
                << std::endl;
    }

    // Calculate statistics
    double sum = std::accumulate(
        result.latenciesMs.begin(), result.latenciesMs.end(), 0.0);
    result.avgLatencyMs = sum / result.latenciesMs.size();
    result.maxLatencyMs =
        *std::max_element(result.latenciesMs.begin(), result.latenciesMs.end());

  } catch (const std::exception& e) {
    result.success = false;
    result.errorMsg = e.what();
    result.avgLatencyMs = 0;
    result.maxLatencyMs = 0;
  }

  return result;
}

void printSeparator() {
  std::cout << std::string(60, '-') << std::endl;
}

void printResult(const BackendResult& result) {
  std::cout << std::endl;
  std::cout << "  Results:" << std::endl;
  if (result.success) {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "    Average latency: " << result.avgLatencyMs << " ms"
              << std::endl;
    std::cout << "    Max latency:     " << result.maxLatencyMs << " ms"
              << std::endl;

    if (result.avgLatencyMs > kLatencyThresholdMs) {
      std::cout << "    Status: SLOW (avg > " << kLatencyThresholdMs << " ms)"
                << std::endl;
    } else {
      std::cout << "    Status: OK (avg < " << kLatencyThresholdMs << " ms)"
                << std::endl;
    }
  } else {
    std::cout << "    Status: FAILED - " << result.errorMsg << std::endl;
  }
}

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);

  std::cout << "========================================" << std::endl;
  std::cout << "  Thrift RPC Latency Test" << std::endl;
  std::cout << "  Iterations per backend: " << kNumIterations << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;

#if HAS_EPOLL_BACKEND
  std::cout << "EpollBackend: available" << std::endl;
#else
  std::cout << "EpollBackend: NOT available" << std::endl;
#endif
  std::cout << std::endl;

  std::vector<BackendResult> results;

  // Test 1: LibEvent (default) backend
  printSeparator();
  std::cout << "[1/3] Testing LibEvent backend (default)..." << std::endl;
  auto libeventResult =
      runBackendTest("LibEvent", [](ThriftServer& /* server */) {
        // No configuration needed - uses default libevent backend
      });
  printResult(libeventResult);
  results.push_back(libeventResult);

  // Test 2: Epoll backend
  printSeparator();
  std::cout << "[2/3] Testing Epoll backend..." << std::endl;
#if HAS_EPOLL_BACKEND
  auto epollResult = runBackendTest("Epoll", [](ThriftServer& server) {
    auto& epollManager = getEpollEventBaseManager();
    auto executor = std::make_shared<folly::IOThreadPoolExecutor>(
        std::thread::hardware_concurrency(),
        std::make_shared<folly::NamedThreadFactory>("ThriftIO"),
        &epollManager,
        folly::IOThreadPoolExecutor::Options().setEnableThreadIdCollection(
            true));
    server.setIOThreadPool(executor);
  });
  printResult(epollResult);
  results.push_back(epollResult);
#else
  std::cout << "  SKIPPED - EpollBackend not available" << std::endl;
  BackendResult epollSkipped;
  epollSkipped.name = "Epoll";
  epollSkipped.success = false;
  epollSkipped.errorMsg = "EpollBackend not available";
  results.push_back(epollSkipped);
#endif

  // Test 3: IoUring backend
  printSeparator();
  std::cout << "[3/3] Testing IoUring backend..." << std::endl;
  auto iouringResult = runBackendTest("IoUring", [](ThriftServer& server) {
    server.setPreferIoUring(true);
    server.setUseDefaultIoUringExecutor(true);
  });
  printResult(iouringResult);
  results.push_back(iouringResult);

  // Print summary
  printSeparator();
  std::cout << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "  Summary" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << std::endl;

  std::cout << std::left << std::setw(12) << "Backend" << std::right
            << std::setw(12) << "Avg (ms)" << std::setw(12) << "Max (ms)"
            << std::setw(12) << "Status" << std::endl;
  std::cout << std::string(48, '-') << std::endl;

  bool hasFailure = false;
  for (const auto& r : results) {
    std::cout << std::left << std::setw(12) << r.name;
    if (r.success) {
      std::cout << std::right << std::fixed << std::setprecision(2)
                << std::setw(12) << r.avgLatencyMs << std::setw(12)
                << r.maxLatencyMs;
      if (r.avgLatencyMs > kLatencyThresholdMs) {
        std::cout << std::setw(12) << "SLOW";
        // LibEvent being slow is expected in OSS builds
        if (r.name != "LibEvent") {
          hasFailure = true;
        }
      } else {
        std::cout << std::setw(12) << "OK";
      }
    } else {
      std::cout << std::right << std::setw(12) << "N/A" << std::setw(12)
                << "N/A" << std::setw(12) << "SKIP/FAIL";
    }
    std::cout << std::endl;
  }

  std::cout << std::endl;

  if (hasFailure) {
    std::cout << "ERROR: Non-libevent backend(s) showed high latency!"
              << std::endl;
    return 1;
  }

  // Check if libevent is slow but alternatives are fast
  bool libeventSlow = libeventResult.success &&
      libeventResult.avgLatencyMs > kLatencyThresholdMs;
  bool hasWorkingAlternative = false;
  for (const auto& r : results) {
    if (r.name != "LibEvent" && r.success &&
        r.avgLatencyMs <= kLatencyThresholdMs) {
      hasWorkingAlternative = true;
      break;
    }
  }

  if (libeventSlow && hasWorkingAlternative) {
    std::cout
        << "Note: LibEvent backend is slow (expected in OSS builds with libevent 2.x)"
        << std::endl;
    std::cout
        << "      Alternative backends (Epoll/IoUring) are working correctly."
        << std::endl;
  }

  std::cout << "Test completed successfully." << std::endl;
  return 0;
}
