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

#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/if/gen-cpp2/MultiNodeTestCtrl.h"

namespace facebook::fboss {

class SwSwitch;

class MultiNodeTestThriftHandler
    : public ThriftHandler,
      public apache::thrift::ServiceHandler<MultiNodeTestCtrl> {
 public:
  explicit MultiNodeTestThriftHandler(SwSwitch* sw);
  ~MultiNodeTestThriftHandler() override = default;

  void triggerGracefulRestart() override;
  void triggerUngracefulRestart() override;

 private:
  // Forbidden copy constructor and assignment operator
  MultiNodeTestThriftHandler(MultiNodeTestThriftHandler const&) = delete;
  MultiNodeTestThriftHandler& operator=(MultiNodeTestThriftHandler const&) =
      delete;
};

} // namespace facebook::fboss
