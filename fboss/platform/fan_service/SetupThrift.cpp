/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "thrift/lib/cpp2/server/ThriftServer.h"

#include "fboss/platform/fan_service/FanService.h"
#include "fboss/platform/fan_service/FanServiceHandler.h"
#include "fboss/platform/fan_service/SetupThrift.h"
#include "fboss/platform/helpers/Init.h"

DEFINE_int32(thrift_port, 5972, "Port for the thrift service");

namespace facebook::fboss::platform {

std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<FanServiceHandler>>
setupThrift() {
  // Define Thrift Server. It will be configured in runSever() function.
  auto server = std::make_shared<apache::thrift::ThriftServer>();
  // Before creating a handler, create Fan Service Object as unique_ptr
  auto fanService = std::make_unique<facebook::fboss::platform::FanService>();
  // Create Thrift Handler (interface)
  // The previously created unique_ptr of fanService will be transferred into
  // the handler This is the interface between FanService and any Thrift call
  // handler to be created
  auto handler = std::make_shared<facebook::fboss::platform::FanServiceHandler>(
      std::move(fanService));
  // Need to run kickstart method in the FanService object,
  // inside the handler.
  handler->getFanService()->kickstart();

  server->setPort(FLAGS_thrift_port);
  server->setInterface(handler);
  server->setAllowPlaintextOnLoopback(true);

  return {server, handler};
}
} // namespace facebook::fboss::platform
