// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include <fboss/fsdb/oper/PathConverter.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include <memory>
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss::fsdb::test {

// Platform-specific implementation is defined in facebook/FsdbTestServer.cpp or
// oss/FsdbTestServer.cpp
extern std::unique_ptr<FsdbTestServerImpl> createPlatformSpecificImpl(
    std::shared_ptr<ServiceHandler> handler,
    uint16_t port);

std::shared_ptr<apache::thrift::ThriftServer> FsdbTestServerImpl::createServer(
    std::shared_ptr<ServiceHandler> handler,
    uint16_t port) {
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setAllowPlaintextOnLoopback(true);
  server->setPort(port);
  server->setInterface(handler);
  server->setSSLPolicy(apache::thrift::SSLPolicy::PERMITTED);
  return server;
}

void FsdbTestServerImpl::checkServerStart(
    std::shared_ptr<apache::thrift::ThriftServer> server,
    uint16_t& fsdbPort) {
  checkWithRetry([&server, &fsdbPort]() {
    fsdbPort = server->getAddress().getPort();
    if (fsdbPort == 0) {
      return false;
    }
    return true;
  });
  CHECK_NE(fsdbPort, 0);
  XLOG(INFO) << "Started thrift server on port " << fsdbPort;
}

FsdbTestServer::FsdbTestServer(
    std::shared_ptr<FsdbConfig> config,
    uint16_t port,
    uint32_t stateSubscriptionServe_ms,
    uint32_t statsSubscriptionServe_ms,
    uint32_t subscriptionServeQueueSize,
    uint32_t statsSubscriptionServeQueueSize)
    : config_(std::move(config)) {
  auto queueSize = std::to_string(subscriptionServeQueueSize);
  gflags::SetCommandLineOptionWithMode(
      "subscriptionServeQueueSize",
      queueSize.c_str(),
      gflags::SET_FLAG_IF_DEFAULT);

  auto statsQueueSize = std::to_string(statsSubscriptionServeQueueSize);
  gflags::SetCommandLineOptionWithMode(
      "statsSubscriptionServeQueueSize",
      statsQueueSize.c_str(),
      gflags::SET_FLAG_IF_DEFAULT);

  auto stateServeInterval = std::to_string(stateSubscriptionServe_ms);
  auto statsServeInterval = std::to_string(statsSubscriptionServe_ms);
  gflags::SetCommandLineOptionWithMode(
      "snapshotInterval", "1s", gflags::SET_FLAG_IF_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "stateSubscriptionServe_ms",
      stateServeInterval.c_str(),
      gflags::SET_FLAG_IF_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "statsSubscriptionServe_ms",
      statsServeInterval.c_str(),
      gflags::SET_FLAG_IF_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "checkOperOwnership", "false", gflags::SET_FLAG_IF_DEFAULT);

  startTestServer(port);
}

FsdbTestServer::~FsdbTestServer() {
  stopTestServer();
}

std::unique_ptr<apache::thrift::Client<FsdbService>>
FsdbTestServer::getClient() {
  return apache::thrift::makeTestClient(handler_);
}

std::string FsdbTestServer::getPublisherId(int publisherIndex) const {
  return folly::to<std::string>("testPublisher-", publisherIndex);
}

std::optional<FsdbOperTreeMetadata> FsdbTestServer::getPublisherRootMetadata(
    const std::string& root,
    bool isStats) const {
  auto pub2Metdata = isStats ? serviceHandler().getStatsPublisherMetadata()
                             : serviceHandler().getStatePublisherMetadata();
  auto metadata = pub2Metdata.getPublisherRootMetadata(root);
  if (metadata) {
    return metadata;
  }
  std::string idRoot;
  if (isStats) {
    idRoot = PathConverter<FsdbOperStatsRoot>::pathToIdTokens({root}).at(0);
  } else {
    idRoot = PathConverter<FsdbOperStateRoot>::pathToIdTokens({root}).at(0);
  }
  return pub2Metdata.getPublisherRootMetadata(idRoot);
}

ServiceHandler::ActiveSubscriptions FsdbTestServer::getActiveSubscriptions()
    const {
  return serviceHandler().getActiveSubscriptions();
}

void FsdbTestServer::startTestServer(uint16_t port) {
  ServiceHandler::Options options;
#ifdef IS_OSS
  // TODO: Enabling serveIdPathSubs causes serve subscriptions to fail with
  // P2015402468
  options.serveIdPathSubs = false;
#else
  options.serveIdPathSubs = true;
#endif
  handler_ = std::make_shared<ServiceHandler>(config_, options);

  impl_ = createPlatformSpecificImpl(handler_, port);
  impl_->startServer(fsdbPort_);
}

void FsdbTestServer::stopTestServer() {
  if (impl_) {
    impl_->stopServer();
  }
  impl_.reset();
}

} // namespace facebook::fboss::fsdb::test
