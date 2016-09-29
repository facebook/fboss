#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "common/init/Init.h"
#include "common/services/cpp/ServiceFramework.h"

#include "fboss/agent/platforms/wedge/WedgeProductInfo.h"
#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManager.h"

using namespace facebook;
using namespace facebook::fboss;

DEFINE_int32(port, 5910, "Port for the thrift service");
DEFINE_string(fruid_filepath, "/dev/shm/fboss/fruid.json",
              "File for storing the fruid data");

std::unique_ptr<TransceiverManager> getManager() {
  auto productInfo = folly::make_unique<WedgeProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto mode = productInfo->getMode();
  if (mode == WedgePlatformMode::WEDGE100) {
    return folly::make_unique<WedgeManager>();
  } else if (
      mode == WedgePlatformMode::GALAXY_LC ||
      mode == WedgePlatformMode::GALAXY_FC) {
    return folly::make_unique<WedgeManager>();
  }
  return folly::make_unique<WedgeManager>();
}

int main(int argc, char **argv) {
  initFacebook(&argc, &argv);

  auto transceiverManager = getManager();
  auto handler = folly::make_unique<QsfpServiceHandler>(
      std::move(transceiverManager));
  handler->init();

  fb303::FacebookBase2* thriftService = handler.get();

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(FLAGS_port);
  server->setInterface(std::move(handler));
  services::ServiceFramework service("QsfpService");

  service.addThriftService(server, thriftService, FLAGS_port);
  service.go();

  return 0;
}
