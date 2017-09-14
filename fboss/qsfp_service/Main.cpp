#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <folly/experimental/FunctionScheduler.h>

#include "fboss/agent/platforms/wedge/WedgeProductInfo.h"
#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include "fboss/qsfp_service/StatsPublisher.h"
#include "fboss/qsfp_service/platforms/wedge/GalaxyManager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge40Manager.h"
#include "fboss/qsfp_service/platforms/wedge/Wedge100Manager.h"

using namespace facebook;
using namespace facebook::fboss;

DEFINE_int32(port, 5910, "Port for the thrift service");
DEFINE_string(fruid_filepath, "/dev/shm/fboss/fruid.json",
              "File for storing the fruid data");

static constexpr auto kStatsPublishInterval =
  std::chrono::milliseconds(std::chrono::minutes(5));

std::unique_ptr<TransceiverManager> getManager() {
  auto productInfo = std::make_unique<WedgeProductInfo>(FLAGS_fruid_filepath);
  productInfo->initialize();
  auto mode = productInfo->getMode();
  if (mode == WedgePlatformMode::WEDGE100) {
    return std::make_unique<Wedge100Manager>();
  } else if (
      mode == WedgePlatformMode::GALAXY_LC ||
      mode == WedgePlatformMode::GALAXY_FC) {
    return std::make_unique<GalaxyManager>();
  }
  return std::make_unique<Wedge40Manager>();
}

int doServerLoop(std::shared_ptr<apache::thrift::ThriftServer>
        thriftServer, std::shared_ptr<QsfpServiceHandler>);
int qsfpServiceInit(int * argc, char*** argv);

int main(int argc, char **argv) {
  qsfpServiceInit(&argc, &argv);

  auto transceiverManager = getManager();
  StatsPublisher publisher(transceiverManager.get());

  auto handler = std::make_shared<QsfpServiceHandler>(
      std::move(transceiverManager));
  handler->init();

  folly::FunctionScheduler scheduler;
  // init after handler has been initted - this ensures everything is setup
  // before we try to retrieve stats for it
  publisher.init();
  scheduler.addFunction(
      [&publisher]() {
        publisher.publishStats();
      },
      kStatsPublishInterval
  );
  // Note: This doesn't block, this merely starts it's own thread
  scheduler.start();

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(FLAGS_port);
  server->setInterface(handler);

  doServerLoop(server, handler);

  return 0;
}
