// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/qsfp_service/QsfpServer.h"

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

DEFINE_int32(port, 5910, "Port for the thrift service");

namespace facebook::fboss {

std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<QsfpServiceHandler>>
setupThriftServer(std::unique_ptr<WedgeManager> transceiverManager) {
  // Create Platform specific FbossMacsecHandler object
  auto macsecHandler = createFbossMacsecHandler(transceiverManager.get());

  auto handler = std::make_shared<QsfpServiceHandler>(
      std::move(transceiverManager), macsecHandler);
  handler->init();
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(FLAGS_port);
  server->setInterface(handler);
  return std::make_pair(server, handler);
}
} // namespace facebook::fboss
