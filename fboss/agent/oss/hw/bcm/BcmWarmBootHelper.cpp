// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

using folly::StringPiece;

namespace facebook { namespace fboss {

BcmWarmBootHelper::BcmWarmBootHelper(int unit, std::string warmBootDir)
    : unit_(unit),
      warmBootDir_(warmBootDir) {}

BcmWarmBootHelper::~BcmWarmBootHelper() {}

void BcmWarmBootHelper::setCanWarmBoot() {}

bool BcmWarmBootHelper::canWarmBoot() {
  return false;
}

}} // facebook::fboss
