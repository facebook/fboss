// Copyright (c) Meta Platforms, Inc. and affiliates.

namespace cpp2 facebook.fboss.platform.platform_manager

include "fboss/platform/platform_manager/platform_manager_snapshot.thrift"

service PlatformManagerService {
  platform_manager_snapshot.PlatformSnapshot getPlatformSnapshot();
}
