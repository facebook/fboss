#include "fboss/fsdb/server/Server.h"

namespace facebook::fboss::fsdb {

void setVersionString() {}

void startThriftServer(
    std::shared_ptr<apache::thrift::ThriftServer> server,
    std::shared_ptr<ServiceHandler> handler) {
  // TODO: run a regular thrift server
}

} // namespace facebook::fboss::fsdb
