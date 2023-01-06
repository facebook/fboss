/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/data_corral_service/SetupDataCorralServiceThrift.h"

#include "thrift/lib/cpp2/server/ThriftServer.h"

#include "fboss/platform/data_corral_service/DataCorralServiceImpl.h"
#include "fboss/platform/data_corral_service/DataCorralServiceThriftHandler.h"
#include "fboss/platform/helpers/Init.h"

namespace facebook::fboss::platform::data_corral_service {

std::pair<
    std::shared_ptr<apache::thrift::ThriftServer>,
    std::shared_ptr<DataCorralServiceThriftHandler>>
setupThrift() {
  // Init DataCorralService
  std::shared_ptr<DataCorralServiceImpl> dataCorralService =
      std::make_shared<DataCorralServiceImpl>();

  // Fetch data once to warmup
  // ToDo dataCorralService->fetchData();

  // Setup thrift handler and server
  return helpers::setupThrift<DataCorralServiceThriftHandler>(
      dataCorralService, FLAGS_thrift_port);
}

} // namespace facebook::fboss::platform::data_corral_service
