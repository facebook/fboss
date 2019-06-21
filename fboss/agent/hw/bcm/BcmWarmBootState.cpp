// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmWarmBootState.h"

#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

namespace facebook {
namespace fboss {

folly::dynamic BcmWarmBootState::hostTableToFollyDynamic() const {
  return hw_->getHostTable()->toFollyDynamic();
}

} // namespace fboss
} // namespace facebook
