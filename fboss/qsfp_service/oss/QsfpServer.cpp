#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <folly/init/Init.h>

using namespace facebook;
using namespace facebook::fboss;

int qsfpServiceInit(int * argc, char *** argv ) {
  folly::init(argc, argv, true);
  return 0;
}

int doServerLoop(std::shared_ptr<apache::thrift::ThriftServer>
        thriftServer, std::shared_ptr<QsfpServiceHandler> /* unused */) {
  thriftServer->setup();
  folly::EventBase * evb = thriftServer->getEventBaseManager()->getEventBase();
  evb->loopForever();
  return 0;
}
