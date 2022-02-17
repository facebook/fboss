// Copyright 2021- Facebook. All rights reserved.

#include "fboss/platform/fan_service/Main.h"
//
// GFLAGS Description : FLAGS_thrift_port
//                      FLAGS_ods_publish_interval
//                      FLAGS_control_interval
//                      FLAGS_config_file
//                      FLAGS_ods_tier
//

DEFINE_int32(thrift_port, 5972, "Thrift Port");
DEFINE_int32(ods_publish_interval, 300, "How often we publish ODS");
DEFINE_int32(
    control_interval,
    5,
    "How often we will read sensors and change fan pwm");
DEFINE_string(config_file, "fan_service.json", "Config File");
DEFINE_string(ods_tier, "ods_router.script", "ODS Tier");
DEFINE_string(mock_input, "", "Mock Input File");
DEFINE_string(mock_output, "", "Mock Output File");

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");

// runServer : a helper function to run Fan Service as Thrift Server.
int runServer(
    std::shared_ptr<apache::thrift::ThriftServer> thriftServer,
    std::shared_ptr<facebook::fboss::platform::FanServiceHandler> handler) {
  facebook::services::ServiceFrameworkLight service("Fan Service");
  thriftServer->setAllowPlaintextOnLoopback(true);
  service.addThriftService(thriftServer, handler.get(), FLAGS_thrift_port);
  service.addModule(
      facebook::services::BuildModule::kModuleName,
      new facebook::services::BuildModule(&service));
  service.addModule(
      facebook::services::ThriftStatsModule::kModuleName,
      new facebook::services::ThriftStatsModule(&service));
  service.addModule(
      facebook::services::Fb303Module::kModuleName,
      new facebook::services::Fb303Module(&service));
  service.addModule(
      facebook::services::AclCheckerModule::kModuleName,
      new facebook::services::AclCheckerModule(&service));
  service.go();
  return 0;
}

int main(int argc, char** argv) {
  // Define the return code
  int rc = 0;

  // Do the Facebook Init
  facebook::initFacebook(&argc, &argv);

  // Parse Flags
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // If Mock configuration is enabled, run Fan Service in Mock mode, then quit.
  // No Thrift service will be created at all.
  if (FLAGS_mock_input != "") {
    // Run as Mock mode
    facebook::fboss::platform::FanService mockedFanService(FLAGS_config_file);
    mockedFanService.kickstart();
    int rc = mockedFanService.runMock(FLAGS_mock_input, FLAGS_mock_output);
    exit(rc);
  }

  // Create Fan Service Object as unique_ptr
  XLOG(INFO) << "Starting FanService as a service config file "
             << FLAGS_config_file;
  auto fanService = std::make_unique<facebook::fboss::platform::FanService>(
      FLAGS_config_file);
  fanService->setOdsTier(FLAGS_ods_tier);

  // Setup Thrift Server. Nothing special.
  // Later, we install our handler in this server
  auto server = std::make_shared<apache::thrift::ThriftServer>();

  // Create Thrift Handler (interface)
  // The previously created unique_ptr of fanService will be transferred into
  // the handler This is the interface between FanService and any Thrift call
  // handler to be created
  auto handler = std::make_shared<facebook::fboss::platform::FanServiceHandler>(
      std::move(fanService));

  // Need to run kickstart method in the FanService object,
  // inside the handler.
  handler->getFanService()->kickstart();

  // Install the handler in Thrift server.
  server->setInterface(handler);

  // Set thrift port.
  server->setPort(FLAGS_thrift_port);

  // Set up scheduler.
  folly::FunctionScheduler scheduler;

  // Add Ods Streamer to the scheduler.
  scheduler.addFunction(
      [handler, server = server]() {
        handler->getFanService()->publishToOds(
            server->getEventBaseManager()->getEventBase());
      },
      std::chrono::seconds(FLAGS_ods_publish_interval),
      "OdsStatsPublish");
  // Add Fan Service control logic to the scheduler.
  scheduler.addFunction(
      [handler]() { handler->getFanService()->controlFan(); },
      std::chrono::seconds(FLAGS_control_interval),
      "FanControl");

  // Finally, start scheduler
  scheduler.start();

  // Also, run the Thrift server
  rc = runServer(server, handler);

  return rc;
}
