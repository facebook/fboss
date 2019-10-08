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

#include <folly/experimental/TestUtil.h>

#include "fboss/agent/platforms/sai/SaiPlatform.h"
namespace facebook::fboss {

class SaiFakePlatform : public SaiPlatform {
 public:
  explicit SaiFakePlatform(std::unique_ptr<PlatformProductInfo> productInfo)
      : SaiPlatform(std::move(productInfo)) {}
  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;
  std::string getHwConfig() override;

  HwAsic* getAsic() const override {
    /* TODO: implement this */
    return nullptr;
  }

 private:
  folly::test::TemporaryDirectory tmpDir_;
};

} // namespace facebook::fboss
