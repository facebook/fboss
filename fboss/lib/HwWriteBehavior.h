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
namespace facebook::fboss {

enum class HwWriteBehavior : int { FAIL, SKIP, WRITE };

/*
 * TODO - make HwWriteBehavior granularity be per swtich
 */
HwWriteBehavior getHwWriteBehavior();

class HwWriteBehaviorRAII {
 public:
  explicit HwWriteBehaviorRAII(HwWriteBehavior behavior);
  ~HwWriteBehaviorRAII();

 private:
  HwWriteBehavior prevBehavior_;
};
}; // namespace facebook::fboss
