// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/MultiSwitchTestServer.h"
#include <thrift/lib/cpp2/async/MultiplexAsyncProcessor.h>
#include "fboss/agent/ThriftHandler.h"

#include <folly/Random.h>

namespace facebook::fboss {

MultiSwitchTestServer::MultiSwitchTestServer(
    std::vector<std::shared_ptr<apache::thrift::AsyncProcessorFactory>>&
        handlers,
    uint16_t port) {
  folly::Baton<> serverStartedBaton;
  thriftThread_ = std::make_unique<std::thread>([=, &serverStartedBaton] {
    // Uniquify SF name for different test runs, since they may run in
    // parallel
    std::string sfName = folly::to<std::string>(
        "swSwitchTestServerSf-", folly::Random::rand32());
    auto server = std::make_shared<apache::thrift::ThriftServer>();
    server->setAllowPlaintextOnLoopback(true);
    if (port) {
      server->setPort(port);
    }
    auto handler =
        std::make_shared<apache::thrift::MultiplexAsyncProcessorFactory>(
            handlers);
    server->setInterface(handler);

// no service framework for OSS.
// TODO - for OSS, call serve() from background thread
#ifndef IS_OSS
    serviceFramework_ = std::make_unique<services::ServiceFrameworkLight>(
        sfName.c_str(),
        true /* threadsafe */,
        services::ServiceFrameworkLight::Options()
            .setDisableScubaLogging(true)
            .setDisableRequestIdLogging(true));
    serviceFramework_->addPrimaryThriftService("FbossTestService", server);
    serviceFramework_->go(false /* waitUntilStop */);
#endif
    swSwitchPort_ = server->getAddress().getPort();
    CHECK_NE(swSwitchPort_, 0);
    serverStartedBaton.post();
  });
  serverStartedBaton.wait();
}

MultiSwitchTestServer::~MultiSwitchTestServer() {
#ifndef IS_OSS
  serviceFramework_->stop();
  serviceFramework_->waitForStop();
#endif
  thriftThread_->join();
  thriftThread_.reset();
}

} // namespace facebook::fboss
