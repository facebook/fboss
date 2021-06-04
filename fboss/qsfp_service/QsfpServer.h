// Copyright 2004-present Facebook. All Rights Reserved.
//

#pragma once

#include <memory>

namespace apache::thrift {
class ThriftServer;
}
namespace facebook::fboss {
class QsfpServiceHandler;

int qsfpServiceInit(int* argc, char*** argv);
int doServerLoop(
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    std::shared_ptr<QsfpServiceHandler> handler);
} // namespace facebook::fboss
