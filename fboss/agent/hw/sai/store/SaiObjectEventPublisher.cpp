// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/store/SaiObjectEventPublisher.h"

#include <folly/Singleton.h>

namespace {
struct singleton_tag_type {};
} // namespace

namespace facebook::fboss {

static folly::Singleton<SaiObjectEventPublisher, singleton_tag_type>
    kSingleton{};

std::shared_ptr<SaiObjectEventPublisher>
SaiObjectEventPublisher::getInstance() {
  return kSingleton.try_get();
}

} // namespace facebook::fboss
