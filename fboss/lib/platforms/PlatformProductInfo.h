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

#include <folly/Range.h>
#include <optional>

#include "fboss/agent/if/gen-cpp2/product_info_types.h"
#include "fboss/lib/platforms/PlatformMode.h"

DECLARE_string(fruid_filepath);

namespace facebook::fboss {

class PlatformProductInfo {
 public:
  explicit PlatformProductInfo(folly::StringPiece path);

  void getInfo(ProductInfo& info) {
    info = productInfo_;
  }
  PlatformMode getMode() const {
    return mode_;
  }
  void initialize();
  std::string getFabricLocation();
  std::string getProductName();

 private:
  // Forbidden copy constructor and assignment operator
  PlatformProductInfo(PlatformProductInfo const&) = delete;
  PlatformProductInfo& operator=(PlatformProductInfo const&) = delete;

  void setFBSerial();
  void initFromFbWhoAmI();
  void initMode();
  void parse(std::string data);

  ProductInfo productInfo_;
  folly::StringPiece path_;
  PlatformMode mode_;
};

/*
 * Convenience API to create product info for fake platforms
 */
std::unique_ptr<PlatformProductInfo> fakeProductInfo();
} // namespace facebook::fboss
