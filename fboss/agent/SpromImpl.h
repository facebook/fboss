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

#include <folly/MacAddress.h>
#include <folly/Range.h>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

namespace facebook { namespace fboss {

class SpromImpl {
 public:

  SpromImpl() {}
  explicit SpromImpl(folly::StringPiece path) {}

  virtual ~SpromImpl() {}

  virtual const std::string& getSupOEM() const = 0;

  virtual const std::string& getSupProduct() const = 0;

  virtual const std::string& getSupSerial() const = 0;

  virtual folly::MacAddress getMacBase() const = 0;

  virtual uint16_t getCardIndex() const = 0;

  virtual const char* getMgmtInterface() const = 0;

  virtual void getInfo(ProductInfo &info) const = 0;

 private:
  // Forbidden copy constructor and assignment operator
  SpromImpl(SpromImpl const &) = delete;
  SpromImpl& operator=(SpromImpl const &) = delete;
};

}} // facebook::fboss
