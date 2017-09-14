#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include <thrift/lib/cpp2/server/ThriftServer.h>

using namespace facebook;
using namespace facebook::fboss;

int qsfpServiceInit(int * /* unused */, char *** /* unused */ ) {
  // no special init needed for open source
  return 0;
}

int doServerLoop(std::shared_ptr<apache::thrift::ThriftServer>
        thriftServer, std::shared_ptr<QsfpServiceHandler> /* unused */) {
  thriftServer->setup();
  folly::EventBase * evb = thriftServer->getEventBaseManager()->getEventBase();
  evb->loopForever();
  return 0;
}
