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

#include <atomic>
#include <memory>

namespace facebook::fboss {

class SdkWrapSettings {
 public:
  enum class WrapBehavior : int { PASS_THRU_SDK_CALLS, BLOCK_SDK_CALLS };
  void setWrapBehavior(WrapBehavior behavior) {
    wrapBehavior_ = behavior;
  }
  WrapBehavior getWrapBehavior() const {
    return wrapBehavior_;
  }
  bool sdkCallsBlocked() const {
    return wrapBehavior_ == WrapBehavior::BLOCK_SDK_CALLS;
  }
  static std::shared_ptr<SdkWrapSettings> getInstance();

 private:
  std::atomic<WrapBehavior> wrapBehavior_{WrapBehavior::PASS_THRU_SDK_CALLS};
};

class SdkBehaviorSetterRAII {
 protected:
  explicit SdkBehaviorSetterRAII(SdkWrapSettings::WrapBehavior desired)
      : prevWrapBehavior_(SdkWrapSettings::getInstance()->getWrapBehavior()) {
    SdkWrapSettings::getInstance()->setWrapBehavior(desired);
  }
  ~SdkBehaviorSetterRAII() {
    SdkWrapSettings::getInstance()->setWrapBehavior(prevWrapBehavior_);
  }

 private:
  const SdkWrapSettings::WrapBehavior prevWrapBehavior_;
};

class SdkBlackHoler : public SdkBehaviorSetterRAII {
 public:
  SdkBlackHoler()
      : SdkBehaviorSetterRAII(SdkWrapSettings::WrapBehavior::BLOCK_SDK_CALLS) {}
};

} // namespace facebook::fboss
