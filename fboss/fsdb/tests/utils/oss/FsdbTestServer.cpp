// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/tests/utils/FsdbTestServer.h"
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss::fsdb::test {

class FsdbTestServerImplOss : public FsdbTestServerImpl {
 public:
  using FsdbTestServerImpl::FsdbTestServerImpl;

  void startServer(uint16_t& fsdbPort) override {
    folly::Baton<> serverStartedBaton;
    server_ = createServer(handler_, port_);

    folly::SocketAddress address;
    address.setFromLocalPort(port_);
    server_->setAddress(address);

    thriftThread_ =
        std::make_unique<std::thread>([this, &serverStartedBaton, &fsdbPort] {
          serverStartedBaton.post();
          server_->serve();
        });
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
