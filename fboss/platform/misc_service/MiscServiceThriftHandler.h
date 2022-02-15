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

#include "fboss/platform/misc_service/MiscServiceImpl.h"
#include "fboss/platform/misc_service/if/gen-cpp2/MiscServiceThrift.h"

#include <memory>
#include <string>
#include <vector>

namespace facebook::fboss::platform::misc_service {

class MiscServiceThriftHandler : public MiscServiceThriftSvIf {
 public:
  explicit MiscServiceThriftHandler(
      std::shared_ptr<MiscServiceImpl> miscService)
      : miscService_(miscService) {}

#if FOLLY_HAS_COROUTINES
  folly::coro::Task<std::unique_ptr<MiscFruidReadResponse>> co_getFruid(
      bool force) override {
    auto response = std::make_unique<MiscFruidReadResponse>();
    getFruid(*response, force);
    co_return response;
  }
#endif
  void getFruid(MiscFruidReadResponse& response, bool force) override;

  MiscServiceImpl* getServiceImpl() {
    return miscService_.get();
  }

 private:
  std::shared_ptr<MiscServiceImpl> miscService_;
};
} // namespace facebook::fboss::platform::misc_service
