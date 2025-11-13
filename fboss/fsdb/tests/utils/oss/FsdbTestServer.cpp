// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include <thrift/lib/cpp/server/TServerEventHandler.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss::fsdb::test {

class ServerReadyEventHandler
    : public apache::thrift::server::TServerEventHandler {
 public:
  explicit ServerReadyEventHandler(folly::Baton<>* baton) : baton_(baton) {}

  void preServe(const folly::SocketAddress* /*address*/) override {
    // Called when server is ready to accept connections
    baton_->post();
  }

 private:
  folly::Baton<>* baton_;
};

class FsdbTestServerImplOss : public FsdbTestServerImpl {
 public:
  using FsdbTestServerImpl::FsdbTestServerImpl;

  void startServer(uint16_t& fsdbPort) override {
    folly::Baton<> serverStartedBaton;
    server_ = createServer(handler_, port_);

    // Listen on IPv6 localhost to match client connection (::1)
    folly::SocketAddress address;
    address.setFromIpPort("::1", port_);
    server_->setAddress(address);

    // Add event handler to signal when server is ready
    auto eventHandler =
        std::make_shared<ServerReadyEventHandler>(&serverStartedBaton);
    server_->addServerEventHandler(eventHandler);

    thriftThread_ = std::make_unique<std::thread>([this] { server_->serve(); });

    // Wait for server to be ready to accept connections
    serverStartedBaton.wait();
    checkServerStart(server_, fsdbPort);
  }

  void stopServer() override {
    if (server_) {
      server_->stop();
    }
    if (thriftThread_ && thriftThread_->joinable()) {
      thriftThread_->join();
    }
    thriftThread_.reset();
  }

 private:
  std::shared_ptr<apache::thrift::ThriftServer> server_;
  std::unique_ptr<std::thread> thriftThread_;
};

std::unique_ptr<FsdbTestServerImpl> createPlatformSpecificImpl(
    std::shared_ptr<ServiceHandler> handler,
    uint16_t port) {
  return std::make_unique<FsdbTestServerImplOss>(handler, port);
}

} // namespace facebook::fboss::fsdb::test
