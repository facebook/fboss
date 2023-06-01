// Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
// All Rights Reserved.

#pragma once

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_service_handlers.h"

namespace facebook::fboss::platform::platform_manager {
class PlatformManagerHandler
    : public apache::thrift::ServiceHandler<PlatformManagerService> {
 public:
  void getPlatformSnapshot(PlatformSnapshot& response) override;
};

} // namespace facebook::fboss::platform::platform_manager
