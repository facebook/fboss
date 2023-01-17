// Copyright 2004-present Facebook. All Rights Reserved.
//
#include "thrift/lib/cpp2/async/AsyncProcessor.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"

namespace facebook::fboss::platform::helpers {

void init(int argc, char** argv);

void runThriftService(
    std::shared_ptr<apache::thrift::ThriftServer> server,
    std::shared_ptr<apache::thrift::ServerInterface> handler,
    const std::string& serviceName,
    uint32_t port);

} // namespace facebook::fboss::platform::helpers
