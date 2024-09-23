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
#include <folly/json/dynamic.h>

#include "fboss/agent/if/gen-cpp2/product_info_types.h"
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"

DECLARE_string(fruid_filepath);

namespace facebook::fboss {

class PlatformProductInfo {
 public:
  explicit PlatformProductInfo(folly::StringPiece path);

  void getInfo(ProductInfo& info) const {
    info = productInfo_;
  }
  PlatformType getType() const {
    return type_;
  }
  void initialize();
  std::string getFabricLocation();
  std::string getProductName();
  int getProductVersion() const;

 private:
  // Forbidden copy constructor and assignment operator
  PlatformProductInfo(PlatformProductInfo const&) = delete;
  PlatformProductInfo& operator=(PlatformProductInfo const&) = delete;

  void setFBSerial();
  void initFromFbWhoAmI();
  void initMode();
  void parse(std::string data);
  /*
   * Check if certain key(s) exists and return its value.
   * BMC and BMC-Lite platforms have different keys
   * for some Information values.
   */
  std::string getField(
      const folly::dynamic& info,
      const std::vector<std::string>& keys);

  ProductInfo productInfo_;
  folly::StringPiece path_;
  PlatformType type_;
};

/*
 * Convenience API to create product info for fake platforms
 */
std::unique_ptr<PlatformProductInfo> fakeProductInfo();
} // namespace facebook::fboss
