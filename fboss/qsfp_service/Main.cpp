#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <folly/experimental/FunctionScheduler.h>
#include <folly/logging/Init.h>

#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include "fboss/qsfp_service/StatsPublisher.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeManagerInit.h"

using namespace facebook;
using namespace facebook::fboss;

DEFINE_int32(port, 5910, "Port for the thrift service");
DEFINE_int32(
    stats_publish_interval,
    300,
    "Interval (in seconds) for publishing stats");
DEFINE_int32(
    loop_interval,
    5,
    "Interval (in seconds) to run the main loop that determines "
    "if we need to change or fetch data for transceivers");

int doServerLoop(std::shared_ptr<apache::thrift::ThriftServer>
        thriftServer, std::shared_ptr<QsfpServiceHandler>);
int qsfpServiceInit(int * argc, char*** argv);

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

int main(int argc, char **argv) {
  qsfpServiceInit(&argc, &argv);

  auto transceiverManager = createTransceiverManager();
  StatsPublisher publisher(transceiverManager.get());

  auto handler = std::make_shared<QsfpServiceHandler>(
    std::move(transceiverManager));
  handler->init();
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(FLAGS_port);
  server->setInterface(handler);
  folly::FunctionScheduler scheduler;
  // init after handler has been initted - this ensures everything is setup
  // before we try to retrieve stats for it
  publisher.init();
  scheduler.addFunction(
      [&publisher, &server]() {
        publisher.publishStats(
            server->getEventBaseManager()->getEventBase(),
            FLAGS_stats_publish_interval);
      },
      std::chrono::seconds(FLAGS_stats_publish_interval),
      "statsPublish");
  scheduler.addFunction(
    [mgr = handler->getTransceiverManager()]() {
      mgr->refreshTransceivers();
    },
    std::chrono::seconds(FLAGS_loop_interval),
    "refreshTransceivers"
  );

  // Schedule the function to periodically send the I2c transaction
  // stats to the ServiceData object which gets pulled by FBagent.
  // The function is called from abstract base class TransceiverManager
  // which gets implemented by platfdorm aware class inheriting
  // this class
  scheduler.addFunction(
    [mgr = handler->getTransceiverManager()]() {
      mgr->publishI2cTransactionStats();
    },
    std::chrono::seconds(FLAGS_loop_interval),
    "publishI2cTransactionStats"
  );

  // Note: This doesn't block, this merely starts it's own thread
  scheduler.start();


  doServerLoop(server, handler);

  return 0;
}
