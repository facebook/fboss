#include <folly/init/Init.h>
#include <folly/io/async/AsyncSignalHandler.h>
#include "NetlinkManagerHandler.h"
#include "common/services/cpp/ServiceFramework.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"

DEFINE_int32(port, 5912, "Port for Netlink Manager Service");

int main(int argc, char** argv) {
  folly::init(&argc, &argv);
  facebook::services::ServiceFramework service("Netlink Manager Service");
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  auto handler = std::make_shared<facebook::fboss::NetlinkManagerHandler>();
  handler->init();
  server->setInterface(handler);
  server->setPort(FLAGS_port);

  service.addThriftService(server, handler.get(), FLAGS_port);
  service.go();
  handler->netlinkManagerThread_->join();

  return 0;
}
