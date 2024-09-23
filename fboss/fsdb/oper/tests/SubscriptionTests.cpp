// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fboss/fsdb/oper/Subscription.h>

#include <folly/coro/BlockingWait.h>
#include <folly/coro/Timeout.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gtest/gtest.h>

namespace facebook::fboss::fsdb::test {

namespace {
template <typename Gen>
folly::coro::Task<typename Gen::value_type> consumeOne(Gen& generator) {
  auto item = co_await generator.next();
  auto&& value = *item;
  co_return std::move(value);
}
} // namespace

template <typename SubscriptionT>
class SubscriptionTests : public ::testing::Test {
 public:
  void SetUp() override {
    heartbeatThread_ = std::make_unique<folly::ScopedEventBaseThread>(
        "SubscriptionHeartbeats");
  }

  auto makeSubscription() {
    std::vector<std::string> path = {"test"};
    return SubscriptionT::create(
        "test-sub",
        path.begin(),
        path.end(),
        OperProtocol::BINARY,
        std::nullopt,
        heartbeatThread_->getEventBase(),
        std::chrono::milliseconds(100));
  }

 private:
  std::shared_ptr<folly::ScopedEventBaseThread> heartbeatThread_;
};

using SimpleSubTypes = ::testing::Types<PathSubscription, DeltaSubscription>;
TYPED_TEST_SUITE(SubscriptionTests, SimpleSubTypes);

TYPED_TEST(SubscriptionTests, verifyHeartbeat) {
  auto [gen, sub] = this->makeSubscription();
  // Should get a heartbeat chunk
  folly::coro::blockingWait(
      folly::coro::timeout(consumeOne(gen), std::chrono::seconds{1}));
}

} // namespace facebook::fboss::fsdb::test
