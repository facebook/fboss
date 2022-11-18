// Copyright 2004-present Facebook. All Rights Reserved.
//
#include "thrift/lib/cpp2/async/AsyncProcessor.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"

namespace facebook::services {
class ServiceFrameworkLight;
}

namespace facebook::fboss::platform::helpers {

void fbInit(int argc, char** argv);

template <typename SvcThriftHandler>
std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<SvcThriftHandler>>
setupThrift(std::shared_ptr<SvcThriftHandler> handler, int thriftPort) {
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(thriftPort);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);

  return {server, handler};
}
template <typename SvcThriftHandler, typename Service>
std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<SvcThriftHandler>>
setupThrift(std::shared_ptr<Service> service, int thriftPort) {
  return setupThrift(std::make_shared<SvcThriftHandler>(service), thriftPort);
}
void addCommonModules(facebook::services::ServiceFrameworkLight& service);

void runThriftService(
    std::shared_ptr<apache::thrift::ThriftServer> server,
    std::shared_ptr<apache::thrift::ServerInterface> handler,
    const std::string& serviceName,
    uint32_t port);

} // namespace facebook::fboss::platform::helpers
