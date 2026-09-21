// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/coro/BlockingWait.h>
#include <gtest/gtest.h>

#include "fboss/fsdb/if/gen-cpp2/FsdbService.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss::fsdb::test {

namespace {
AddPatchSubscriptionPathsRequest makeRequest(
    const std::string& instanceId,
    int64_t uid,
    std::map<SubscriptionKey, RawOperPath> paths,
    StreamRevision lastStreamRevision = 0) {
  AddPatchSubscriptionPathsRequest req;
  req.clientId()->instanceId() = instanceId;
  req.subscriptionUid() = uid;
  req.paths() = std::move(paths);
  req.lastStreamRevision() = lastStreamRevision;
  return req;
}

AddPatchSubscriptionPathsRequest makeExtRequest(
    const std::string& instanceId,
    int64_t uid,
    std::map<SubscriptionKey, ExtendedOperPath> extPaths,
    StreamRevision lastStreamRevision = 0) {
  AddPatchSubscriptionPathsRequest req;
  req.clientId()->instanceId() = instanceId;
  req.subscriptionUid() = uid;
  req.extPaths() = std::move(extPaths);
  req.lastStreamRevision() = lastStreamRevision;
  return req;
}
} // namespace

class FsdbAddPatchSubscriptionPathsTest : public ::testing::Test {
 protected:
  FsdbTestServer server_;
};

// Empty path map is rejected with INVALID_REQUEST, confirming the new RPC is
// wired through the handler and maps a storage error to an FsdbException.
TEST_F(FsdbAddPatchSubscriptionPathsTest, EmptyPathsRejected) {
  auto client = server_.getClient();
  auto req = makeRequest("noSub", /*uid=*/1, /*paths=*/{});
  try {
    folly::coro::blockingWait(client->co_addStatePatchSubscriptionPaths(req));
    FAIL() << "expected FsdbException";
  } catch (const FsdbException& e) {
    EXPECT_EQ(*e.errorCode(), FsdbErrorCode::INVALID_REQUEST);
  }
}

// Adding paths to a non-existent subscription is rejected with ID_NOT_FOUND.
TEST_F(FsdbAddPatchSubscriptionPathsTest, UnknownSubscriptionNotFound) {
  auto client = server_.getClient();
  RawOperPath p;
  p.path() = std::vector<std::string>{"agent"};
  auto req =
      makeRequest("noSuchSubscriber", /*uid=*/12345, {{1, std::move(p)}});
  try {
    folly::coro::blockingWait(client->co_addStatePatchSubscriptionPaths(req));
    FAIL() << "expected FsdbException";
  } catch (const FsdbException& e) {
    EXPECT_EQ(*e.errorCode(), FsdbErrorCode::ID_NOT_FOUND);
  }
}

// Stats tree variant: unknown subscription is rejected with ID_NOT_FOUND.
TEST_F(FsdbAddPatchSubscriptionPathsTest, UnknownStatsSubscriptionNotFound) {
  auto client = server_.getClient();
  RawOperPath p;
  p.path() = std::vector<std::string>{"agent"};
  auto req =
      makeRequest("noSuchSubscriber", /*uid=*/12345, {{1, std::move(p)}});
  try {
    folly::coro::blockingWait(client->co_addStatsPatchSubscriptionPaths(req));
    FAIL() << "expected FsdbException";
  } catch (const FsdbException& e) {
    EXPECT_EQ(*e.errorCode(), FsdbErrorCode::ID_NOT_FOUND);
  }
}

// Positive end-to-end: open a live patch subscription, capture the server uid
// from OperSubInitResponse, then append a raw path and confirm the RPC is
// accepted. Data delivery for appended paths is covered via FsdbSubManager.
TEST_F(FsdbAddPatchSubscriptionPathsTest, AddPathsToLiveSubscription) {
  auto client = server_.getClient();

  // Open a patch subscription on "agent"; forceSubscribe so it registers even
  // without a connected publisher.
  SubRequest subReq;
  RawOperPath subPath;
  subPath.path() = std::vector<std::string>{"agent"};
  subReq.paths() = {{0, std::move(subPath)}};
  subReq.clientId()->instanceId() = "liveSub";
  subReq.forceSubscribe() = true;
  auto subResult = folly::coro::blockingWait(client->co_subscribeState(subReq));

  // The server returns a non-empty subscriptionUid that the client echoes back
  // when appending paths.
  ASSERT_TRUE(subResult.response.subscriptionUid().has_value());
  auto uid = *subResult.response.subscriptionUid();

  // Hold the stream open so the subscription stays registered while we append.
  auto stream = std::move(subResult.stream);

  // Append a second raw path (distinct key) under the same publisher root;
  // retry only while the subscription is still settling (ID_NOT_FOUND).
  RawOperPath addPath;
  addPath.path() = std::vector<std::string>{"agent"};
  auto addReq = makeRequest("liveSub", uid, {{1, std::move(addPath)}});
  WITH_RETRIES({
    std::optional<FsdbErrorCode> errCode;
    try {
      folly::coro::blockingWait(
          client->co_addStatePatchSubscriptionPaths(addReq));
    } catch (const FsdbException& e) {
      errCode = *e.errorCode();
    }
    // Keep retrying only until the subscription has finished registering.
    bool settled = errCode != FsdbErrorCode::ID_NOT_FOUND;
    ASSERT_EVENTUALLY_TRUE(settled);
    EXPECT_FALSE(errCode.has_value())
        << "unexpected addPatchSubscriptionPaths error: "
        << static_cast<int>(*errCode);
  });
}

// Extended paths: empty extPaths map is rejected.
TEST_F(FsdbAddPatchSubscriptionPathsTest, EmptyExtPathsRejected) {
  auto client = server_.getClient();
  auto req = makeExtRequest("noSub", /*uid=*/1, /*extPaths=*/{});
  try {
    folly::coro::blockingWait(client->co_addStatePatchSubscriptionPaths(req));
    FAIL() << "expected FsdbException";
  } catch (const FsdbException& e) {
    EXPECT_EQ(*e.errorCode(), FsdbErrorCode::INVALID_REQUEST);
  }
}

// Extended paths: unknown subscription is rejected with ID_NOT_FOUND.
TEST_F(FsdbAddPatchSubscriptionPathsTest, ExtPathsUnknownSubscriptionNotFound) {
  auto client = server_.getClient();
  ExtendedOperPath ep;
  OperPathElem elem;
  elem.set_raw("agent");
  ep.path() = std::vector<OperPathElem>{std::move(elem)};
  auto req =
      makeExtRequest("noSuchSubscriber", /*uid=*/12345, {{1, std::move(ep)}});
  try {
    folly::coro::blockingWait(client->co_addStatePatchSubscriptionPaths(req));
    FAIL() << "expected FsdbException";
  } catch (const FsdbException& e) {
    EXPECT_EQ(*e.errorCode(), FsdbErrorCode::ID_NOT_FOUND);
  }
}

// Positive end-to-end for extended paths: open a live patch subscription,
// capture the server uid, then append a valid extended path and confirm the
// extended RPC branch is accepted.
TEST_F(FsdbAddPatchSubscriptionPathsTest, AddExtPathsToLiveSubscription) {
  auto client = server_.getClient();

  // Open a patch subscription on "agent"; forceSubscribe so it registers even
  // without a connected publisher.
  SubRequest subReq;
  RawOperPath subPath;
  subPath.path() = std::vector<std::string>{"agent"};
  subReq.paths() = {{0, std::move(subPath)}};
  subReq.clientId()->instanceId() = "liveExtSub";
  subReq.forceSubscribe() = true;
  auto subResult = folly::coro::blockingWait(client->co_subscribeState(subReq));

  ASSERT_TRUE(subResult.response.subscriptionUid().has_value());
  auto uid = *subResult.response.subscriptionUid();

  // Hold the stream open so the subscription stays registered while we append.
  auto stream = std::move(subResult.stream);

  // Append a valid extended path (distinct key) under the same publisher root;
  // retry only while the subscription is still settling (ID_NOT_FOUND).
  ExtendedOperPath ep;
  OperPathElem elem;
  elem.set_raw("agent");
  ep.path() = std::vector<OperPathElem>{std::move(elem)};
  auto addReq = makeExtRequest("liveExtSub", uid, {{1, std::move(ep)}});
  WITH_RETRIES({
    std::optional<FsdbErrorCode> errCode;
    try {
      folly::coro::blockingWait(
          client->co_addStatePatchSubscriptionPaths(addReq));
    } catch (const FsdbException& e) {
      errCode = *e.errorCode();
    }
    // Keep retrying only until the subscription has finished registering.
    bool settled = errCode != FsdbErrorCode::ID_NOT_FOUND;
    ASSERT_EVENTUALLY_TRUE(settled);
    EXPECT_FALSE(errCode.has_value())
        << "unexpected addPatchSubscriptionPaths error: "
        << static_cast<int>(*errCode);
  });
}

// Both paths and extPaths set → INVALID_REQUEST (mutual exclusivity).
TEST_F(FsdbAddPatchSubscriptionPathsTest, BothPathsAndExtPathsRejected) {
  auto client = server_.getClient();
  AddPatchSubscriptionPathsRequest req;
  req.clientId()->instanceId() = "sub";
  req.subscriptionUid() = 1;
  RawOperPath p;
  p.path() = std::vector<std::string>{"agent"};
  req.paths() = {{1, std::move(p)}};
  ExtendedOperPath ep;
  OperPathElem elem;
  elem.set_raw("agent");
  ep.path() = std::vector<OperPathElem>{std::move(elem)};
  req.extPaths() = {{2, std::move(ep)}};
  try {
    folly::coro::blockingWait(client->co_addStatePatchSubscriptionPaths(req));
    FAIL() << "expected FsdbException";
  } catch (const FsdbException& e) {
    EXPECT_EQ(*e.errorCode(), FsdbErrorCode::INVALID_REQUEST);
  }
}

} // namespace facebook::fboss::fsdb::test
