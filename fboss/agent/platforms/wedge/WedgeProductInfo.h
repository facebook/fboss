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

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include <folly/Range.h>

namespace facebook { namespace fboss {

class WedgeProductInfo {
 public:
  explicit WedgeProductInfo(folly::StringPiece path);

  void getInfo(ProductInfo& info);
  void initialize();

 private:
  // Forbidden copy constructor and assignment operator
  WedgeProductInfo(WedgeProductInfo const &) = delete;
  WedgeProductInfo& operator=(WedgeProductInfo const &) = delete;

  void parse(std::string data);
  ProductInfo productInfo_;
  folly::StringPiece path_;
};

}} // facebook::fboss
