#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "common/services/cpp/ServiceFramework.h"
#include "common/init/Init.h"

#include "fboss/qsfp_service/QsfpServiceHandler.h"

using namespace facebook;
using namespace facebook::fboss;

DEFINE_int32(port, 5910, "Port for the thrift service");

int main(int argc, char **argv) {
  initFacebook(&argc, &argv);

  auto handler = folly::make_unique<QsfpServiceHandler>();

  fb303::FacebookBase2* thriftService = handler.get();

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(FLAGS_port);
  server->setInterface(std::move(handler));
  services::ServiceFramework service("QsfpService");

  service.addThriftService(server, thriftService, FLAGS_port);
  service.go();

  return 0;
}
