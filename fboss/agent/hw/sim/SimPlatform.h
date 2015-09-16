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

#include "fboss/agent/Platform.h"

namespace facebook { namespace fboss {

class SimSwitch;

class SimPlatform : public Platform {
 public:
  SimPlatform(folly::MacAddress mac, uint32_t numPorts);
  ~SimPlatform() override;

  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(SwSwitch* sw) override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;

  folly::MacAddress getLocalMac() const override {
    return mac_;
  }
  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;
  void getProductInfo(ProductInfo& info) override {
    // Nothing to do
  };

 private:
  // Forbidden copy constructor and assignment operator
  SimPlatform(SimPlatform const &) = delete;
  SimPlatform& operator=(SimPlatform const &) = delete;

  folly::MacAddress mac_;
  std::unique_ptr<SimSwitch> hw_;
};

}} // facebook::fboss
