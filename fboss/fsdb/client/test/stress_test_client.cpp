// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <iostream>
#include <thread>

#include <folly/String.h>
#include <folly/init/Init.h>
#include <folly/synchronization/Baton.h>
#include <gflags/gflags.h>
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/client/instantiations/FsdbCowStateSubManager.h"
#include "fboss/fsdb/client/instantiations/FsdbCowStatsSubManager.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_constants.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

DEFINE_int32(
    consumeDelayMs,
    0,
    "Delay in milliseconds before consuming each chunk");

DEFINE_int32(
    resetDelayAfterNChunks,
    0,
    "Reset delay to 0 after consuming N chunks");

DEFINE_bool(printChunks, false, "print chunks to stdout");

DEFINE_bool(count, false, "Print count of received chunks");

DEFINE_bool(stats, false, "Subscribe to stats instead of state");

DEFINE_string(host, "::1", "FSDB server host");

DEFINE_int32(
    port,
    facebook::fboss::fsdb::fsdb_common_constants::PORT(),
    "FSDB server port");

using namespace facebook::fboss::fsdb;

facebook::fboss::utils::ConnectionOptions getConnectionOptions() {
  return facebook::fboss::utils::ConnectionOptions(FLAGS_host, FLAGS_port);
}

// Wrapper to allow using addPath() with runtime string paths
template <bool IsStats>
struct RawPath {
  using RootT =
      std::conditional_t<IsStats, FsdbOperStatsRoot, FsdbOperStateRoot>;

  explicit RawPath(std::vector<std::string> tokens)
      : tokens_(std::move(tokens)) {}

  std::vector<std::string> idTokens() const {
    return tokens_;
  }

 private:
  std::vector<std::string> tokens_;
};

template <typename T>
void consume(
    const T& /* chunk */,
    int32_t delayMs,
    int32_t resetDelayAfterNChunks) {
  static int nChunksConsumed{0};
  nChunksConsumed++;
  if (FLAGS_count) {
    std::cout << "Received chunk " << nChunksConsumed << std::endl;
  }
  if ((delayMs > 0) && (nChunksConsumed <= resetDelayAfterNChunks)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
  }
}

void subscribe(const std::vector<std::string>& rawPath, bool subscribeStats) {
  auto pubSubMgr =
      std::make_unique<FsdbPubSubManager>("stress_test_client_path");

  auto connectionOptions = getConnectionOptions();

  folly::Baton<> baton;

  auto stateChangeCb = [](SubscriptionState oldState,
                          SubscriptionState newState,
                          std::optional<bool> /* initialSyncHasData */) {
    std::cout << "Path subscription state changed: "
              << static_cast<int>(oldState) << " -> "
              << static_cast<int>(newState) << std::endl;
  };

  auto operStateCb = [](OperState&& state) {
    consume(state, FLAGS_consumeDelayMs, FLAGS_resetDelayAfterNChunks);
    if (FLAGS_printChunks) {
      auto contents = state.contents() ? *state.contents() : "null";
      std::cout << "New value: " << contents << std::endl;
    }
  };

  if (subscribeStats) {
    pubSubMgr->addStatPathSubscription(
        rawPath,
        std::move(stateChangeCb),
        std::move(operStateCb),
        std::move(connectionOptions));
    std::cout
        << "Subscribed via FsdbPubSubManager (stat path). Press Ctrl+C to exit."
        << std::endl;
  } else {
    pubSubMgr->addStatePathSubscription(
        rawPath,
        std::move(stateChangeCb),
        std::move(operStateCb),
        std::move(connectionOptions));
    std::cout
        << "Subscribed via FsdbPubSubManager (state path). Press Ctrl+C to exit."
        << std::endl;
  }

  baton.wait();
}

void subscribeDelta(
    const std::vector<std::string>& rawPath,
    bool subscribeStats) {
  auto pubSubMgr =
      std::make_unique<FsdbPubSubManager>("stress_test_client_delta");

  auto connectionOptions = getConnectionOptions();

  folly::Baton<> baton;

  auto stateChangeCb = [](SubscriptionState oldState,
                          SubscriptionState newState,
                          std::optional<bool> /* initialSyncHasData */) {
    std::cout << "Delta subscription state changed: "
              << static_cast<int>(oldState) << " -> "
              << static_cast<int>(newState) << std::endl;
  };

  auto operDeltaCb = [](OperDelta&& delta) {
    consume(delta, FLAGS_consumeDelayMs, FLAGS_resetDelayAfterNChunks);
    if (FLAGS_printChunks) {
      std::cout << "New Delta: " << std::endl;
      for (auto& change : *delta.changes()) {
        if (change.newState()) {
          std::cout << "\t/" << folly::join("/", *change.path()->raw()) << ": "
                    << *change.newState() << std::endl;
        } else {
          std::cout << "\tDeleted /" << folly::join("/", *change.path()->raw())
                    << std::endl;
        }
      }
    }
  };

  if (subscribeStats) {
    pubSubMgr->addStatDeltaSubscription(
        rawPath,
        std::move(stateChangeCb),
        std::move(operDeltaCb),
        std::move(connectionOptions));
    std::cout
        << "Subscribed via FsdbPubSubManager (stat delta). Press Ctrl+C to exit."
        << std::endl;
  } else {
    pubSubMgr->addStateDeltaSubscription(
        rawPath,
        std::move(stateChangeCb),
        std::move(operDeltaCb),
        std::move(connectionOptions));
    std::cout
        << "Subscribed via FsdbPubSubManager (state delta). Press Ctrl+C to exit."
        << std::endl;
  }

  baton.wait();
}

void subscribePatch(const std::vector<std::string>& rawPath, bool isStats) {
  auto connectionOptions = getConnectionOptions();

  folly::Baton<> baton;

  auto stateChangeCb = [](SubscriptionState oldState,
                          SubscriptionState newState,
                          std::optional<bool> /* initialSyncHasData */) {
    std::cout << "Subscription state changed: " << static_cast<int>(oldState)
              << " -> " << static_cast<int>(newState) << std::endl;
  };

  auto doSubscribe = [&]<typename SubManagerT, bool IsStats>() {
    SubscriptionOptions options("stress_test_client", isStats);
    auto subscriber = std::make_unique<SubManagerT>(
        std::move(options), std::move(connectionOptions));

    RawPath<IsStats> path(rawPath);
    subscriber->addPath(std::move(path));

    subscriber->subscribe(
        [&](auto update) {
          consume(update, FLAGS_consumeDelayMs, FLAGS_resetDelayAfterNChunks);
          if (FLAGS_printChunks) {
            std::cout << "Received patch update:" << std::endl;
            std::cout << "  Updated paths: " << update.updatedPaths.size()
                      << std::endl;
            for (const auto& p : update.updatedPaths) {
              std::cout << "    /" << folly::join("/", p) << std::endl;
            }
            if (update.lastServedAt.has_value()) {
              std::cout << "  Last served at: " << *update.lastServedAt
                        << std::endl;
            }
            if (update.lastPublishedAt.has_value()) {
              std::cout << "  Last published at: " << *update.lastPublishedAt
                        << std::endl;
            }
          }
        },
        std::move(stateChangeCb));

    std::cout << "Subscribed via FsdbSubManager ("
              << (isStats ? "stat" : "state")
              << " patch). Press Ctrl+C to exit." << std::endl;
    baton.wait();
  };

  if (isStats) {
    doSubscribe.template operator()<FsdbCowStatsSubManager, true>();
  } else {
    doSubscribe.template operator()<FsdbCowStateSubManager, false>();
  }
}

int main(int argc, char** argv) {
  const folly::Init init(&argc, &argv);

  if (argc < 3) {
    std::cout << "Incorrect usage. Expected 2 arguments, streamType and path"
              << std::endl;
    std::cout
        << "Usage: stress_test_client <streamType> <path> [--stats] [--consumeDelayMs=<ms>] [--resetDelayAfterNChunks=<n>] [--count]"
        << std::endl;
    std::cout << "streamType: subscribePath, subscribeDelta, subscribePatch"
              << std::endl;
    return 1;
  }

  try {
    std::string streamType(argv[1]);
    std::string pathStr(argv[2]);

    std::vector<std::string> rawPath;
    folly::split('#', pathStr, rawPath);

    if (streamType == "subscribePath") {
      subscribe(std::move(rawPath), FLAGS_stats);
    } else if (streamType == "subscribeDelta") {
      subscribeDelta(std::move(rawPath), FLAGS_stats);
    } else if (streamType == "subscribePatch") {
      subscribePatch(std::move(rawPath), FLAGS_stats);
    } else {
      std::cout << "Incorrect usage. Choose from 'subscribePath', "
                << "'subscribeDelta', or 'subscribePatch'" << std::endl;
      return 1;
    }
  } catch (const FsdbException& e) {
    std::cerr << "errorCode=" << apache::thrift::util::enumName(*e.errorCode())
              << ", error=" << *e.message() << std::endl;
    return 5;
  }
}
