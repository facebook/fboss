/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/misc_service/SetupMiscServiceThrift.h"

#include "common/services/cpp/ServiceFrameworkLight.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/misc_service/MiscServiceImpl.h"
#include "fboss/platform/misc_service/MiscServiceThriftHandler.h"

DEFINE_int32(thrift_port, 5971, "Port for the thrift service");

namespace facebook::fboss::platform::misc_service {

std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<MiscServiceThriftHandler>>
setupThrift() {
  // Init MiscService
  std::shared_ptr<MiscServiceImpl> miscService =
      std::make_shared<MiscServiceImpl>();

  // Fetch data once to warmup
  // ToDo miscService->fetchData();

  // Setup thrift handler and server
  return helpers::setupThrift<MiscServiceThriftHandler>(
      miscService, FLAGS_thrift_port);
}

} // namespace facebook::fboss::platform::misc_service
