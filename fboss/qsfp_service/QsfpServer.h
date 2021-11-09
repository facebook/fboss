// Copyright 2004-present Facebook. All Rights Reserved.
//

#pragma once

#include <gflags/gflags.h>
#include <memory>

DECLARE_int32(port);

namespace apache::thrift {
class ThriftServer;
}
namespace facebook::fboss {
class QsfpServiceHandler;
class WedgeManager;

void setVersionInfo();
int qsfpServiceInit(int* argc, char*** argv);
std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<QsfpServiceHandler>>
setupThriftServer(std::unique_ptr<WedgeManager> transceiverManager);
int doServerLoop(
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    std::shared_ptr<QsfpServiceHandler> handler);
} // namespace facebook::fboss
