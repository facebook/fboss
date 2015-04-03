// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"

using folly::StringPiece;

namespace facebook { namespace fboss {

BcmWarmBootHelper::BcmWarmBootHelper(int unit, StringPiece warmBootDir)
    : unit_(unit),
      warmBootDir_(warmBootDir) {}

BcmWarmBootHelper::~BcmWarmBootHelper() {}

void BcmWarmBootHelper::setCanWarmBoot() {}

bool BcmWarmBootHelper::canWarmBoot() {
  return false;
}

/*std::string BcmWarmBootHelper::warmBootFlag() const {
  return "";
}

std::string BcmWarmBootHelper::warmBootDataPath() const {
  return "";
}

std::string BcmWarmBootHelper::forceColdBootOnceFlag() const {
  return "";
}

void BcmWarmBootHelper::setupWarmBootFile() {}

bool BcmWarmBootHelper::checkAndClearWarmBootFlags() {}

void BcmWarmBootHelper::warmBootRead(uint8_t* buf, int offset, int nbytes) {}

void BcmWarmBootHelper::warmBootWrite(const uint8_t* buf,
                                    int offset, int nbytes) {}

int BcmWarmBootHelper::warmBootReadCallback(int unitNumber, uint8_t* buf,
                                          int offset, int nbytes) {}

int BcmWarmBootHelper::warmBootWriteCallback(int unitNumber, uint8_t* buf,
                                           int offset, int nbytes) {}
*/
}} // facebook::fboss
