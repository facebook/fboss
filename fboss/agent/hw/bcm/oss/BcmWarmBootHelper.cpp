// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

namespace facebook { namespace fboss {

void DiscBackedBcmWarmBootHelper::setupWarmBootFile() {}
void DiscBackedBcmWarmBootHelper::warmBootRead(
    uint8_t* /*buf*/,
    int /*offset*/,
    int /*nbytes*/) {}

void DiscBackedBcmWarmBootHelper::warmBootWrite(
    const uint8_t* /*buf*/,
    int /*offset*/,
    int /*nbytes*/) {}
}} // facebook::fboss
