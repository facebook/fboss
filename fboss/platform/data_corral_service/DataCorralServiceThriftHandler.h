/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/platform/data_corral_service/DataCorralServiceImpl.h"
#include "fboss/platform/data_corral_service/if/gen-cpp2/DataCorralServiceThrift.h"

#include <memory>
#include <string>
#include <vector>

namespace facebook::fboss::platform::data_corral_service {

class DataCorralServiceThriftHandler : public DataCorralServiceThriftSvIf {
 public:
  explicit DataCorralServiceThriftHandler(
      std::shared_ptr<DataCorralServiceImpl> dataCorralService)
      : dataCorralService_(dataCorralService) {}

#if FOLLY_HAS_COROUTINES
  folly::coro::Task<std::unique_ptr<DataCorralFruidReadResponse>> co_getFruid(
      bool force) override {
    auto response = std::make_unique<DataCorralFruidReadResponse>();
    getFruid(*response, force);
    co_return response;
  }
#endif
  void getFruid(DataCorralFruidReadResponse& response, bool force) override;

  DataCorralServiceImpl* getServiceImpl() {
    return dataCorralService_.get();
  }

 private:
  std::shared_ptr<DataCorralServiceImpl> dataCorralService_;
};
} // namespace facebook::fboss::platform::data_corral_service
