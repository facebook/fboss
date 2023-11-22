// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include <fboss/fsdb/oper/PathConverter.h>
#include <memory>
#include "common/services/cpp/ServiceFrameworkLight.h"
#include "common/strings/UUID.h"

namespace facebook::fboss::fsdb::test {

FsdbTestServer::FsdbTestServer(
    std::unique_ptr<FsdbConfig> config,
    uint16_t port) {
  // Run tests faster
  gflags::SetCommandLineOptionWithMode(
      "snapshotInterval", "1s", gflags::SET_FLAG_IF_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "statsSubscriptionServe", "1s", gflags::SET_FLAG_IF_DEFAULT);
  gflags::SetCommandLineOptionWithMode(
      "checkOperOwnership", "false", gflags::SET_FLAG_IF_DEFAULT);

  // Pick different dir for tests
  tmpDir_ = std::make_unique<folly::test::TemporaryDirectory>("fsdb_tests");
  gflags::SetCommandLineOptionWithMode(
      "rocksdbDir",
      tmpDir_->path().string().c_str(),
      gflags::SET_FLAG_IF_DEFAULT);

  folly::Baton<> serverStartedBaton;
  thriftThread_ =
      std::make_unique<std::thread>([=, &serverStartedBaton, &config] {
        std::vector<std::string> publisherIdsToOpenRocksDbAtStartFor;
        for (auto i = 0; i < kMaxPublishers_; ++i) {
          publisherIdsToOpenRocksDbAtStartFor.push_back(getPublisherId(i));
        }

        ServiceHandler::Options options;
        options.eraseRocksDbsInCtorAndDtor_CAUTION_DO_NOT_USE_IN_PRODUCTION =
            true;
        options.serveIdPathSubs = true;

        handler_ = std::make_shared<ServiceHandler>(
            std::move(config),
            folly::join(",", publisherIdsToOpenRocksDbAtStartFor),
            options);
        // Uniquify SF name for different test runs, since they may run in
        // parallel
        std::string sfName = folly::to<std::string>(
            "fsdbTestServerSf-", strings::generateUUID());
        serviceFramework_ = std::make_unique<services::ServiceFrameworkLight>(
            sfName.c_str(),
            true /* threadsafe */,
            services::ServiceFrameworkLight::Options()
                .setDisableScubaLogging(true)
                .setDisableRequestIdLogging(true));
        auto server = std::make_shared<apache::thrift::ThriftServer>();
        server->setAllowPlaintextOnLoopback(true);
        server->setPort(port);
        server->setInterface(handler_);
        server->setSSLPolicy(apache::thrift::SSLPolicy::PERMITTED);
        serviceFramework_->addThriftService(
            server,
            handler_.get(),
            0 /* port */,
            services::ServiceFrameworkLight::ServerOptions()
                .setExportUnprefixedCounters(false));
        serviceFramework_->go(false /* waitUntilStop */);
        fsdbPort_ = server->getAddress().getPort();
        CHECK_NE(fsdbPort_, 0);
        serverStartedBaton.post();
      });
  serverStartedBaton.wait();
}

FsdbTestServer::~FsdbTestServer() {
  serviceFramework_->stop();
  serviceFramework_->waitForStop();
  thriftThread_->join();
  thriftThread_.reset();
  tmpDir_.reset();
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
  // try retrieving using id paths
  std::string idRoot;
  if (isStats) {
    idRoot = PathConverter<FsdbOperStatsRoot>::pathToIdTokens({root}).at(0);
  } else {
    idRoot = PathConverter<FsdbOperStateRoot>::pathToIdTokens({root}).at(0);
  }
  return pub2Metdata.getPublisherRootMetadata(idRoot);
}

std::set<OperSubscriberInfo> FsdbTestServer::getActiveSubscriptions() const {
  return serviceHandler().getActiveSubscriptions();
}

} // namespace facebook::fboss::fsdb::test
