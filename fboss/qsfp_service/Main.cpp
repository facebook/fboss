#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <folly/experimental/FunctionScheduler.h>

#include "fboss/qsfp_service/Main.h"
#include "fboss/qsfp_service/QsfpServiceHandler.h"
#include "fboss/qsfp_service/StatsPublisher.h"

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

int qsfpMain(int* argc, char ***argv, ManagerInitFn createTransceiverManager) {
  qsfpServiceInit(argc, argv);

  auto transceiverManager = createTransceiverManager();
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
      std::chrono::seconds(FLAGS_stats_publish_interval),
      "statsPublish"
  );
  scheduler.addFunction(
    [mgr = handler->getTransceiverManager()]() {
      mgr->refreshTransceivers();
    },
    std::chrono::seconds(FLAGS_loop_interval),
    "refreshTransceivers"
  );
  // Note: This doesn't block, this merely starts it's own thread
  scheduler.start();

  auto server = std::make_shared<apache::thrift::ThriftServer>();
  server->setPort(FLAGS_port);
  server->setInterface(handler);

  doServerLoop(server, handler);

  return 0;
}
