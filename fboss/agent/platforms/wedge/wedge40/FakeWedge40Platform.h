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

#include "fboss/agent/platforms/wedge/wedge40/Wedge40Platform.h"

#include <folly/experimental/TestUtil.h>

namespace facebook::fboss {

class FakeWedge40Platform : public Wedge40Platform {
 public:
  using Wedge40Platform::Wedge40Platform;

  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;
  bool isBcmShellSupported() const override {
    return false;
  }
  const AgentDirectoryUtil* getDirectoryUtil() const override {
    return agentDirUtil_.get();
  }

 private:
  // Forbidden copy constructor and assignment operator
  FakeWedge40Platform(FakeWedge40Platform const&) = delete;
  FakeWedge40Platform& operator=(FakeWedge40Platform const&) = delete;

  folly::test::TemporaryDirectory tmpDir_;
  std::unique_ptr<AgentDirectoryUtil> agentDirUtil_{new AgentDirectoryUtil(
      tmpDir_.path().string() + "/volatile",
      tmpDir_.path().string() + "/persist")};
};

} // namespace facebook::fboss
