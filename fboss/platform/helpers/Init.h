// Copyright 2004-present Facebook. All Rights Reserved.
//
#include "thrift/lib/cpp2/server/ThriftServer.h"

namespace facebook::fboss::platform::helpers {

void fbInit(int argc, char** argv);

template <typename SvcThriftHandler, typename Service>
std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<SvcThriftHandler>>
setupThrift(std::shared_ptr<Service> service, int thriftPort) {
  auto handler = std::make_shared<SvcThriftHandler>(service);
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(thriftPort);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);

  return {server, handler};
}
} // namespace facebook::fboss::platform::helpers
