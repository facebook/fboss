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

#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/Platform.h"

namespace facebook { namespace fboss {

/*
 * MockablePlatform is a mockable wrapper of a Platform. It exposes
 * the same interface as MockPlatform, but instead of pure mocks +
 * stubbed out methods, it invokes corresponding functions on the
 * real platform by default.
 *
 * Note that this is true for mocked gmock functions as well. Those
 * functions will invoke the same function on the real hwswitch by
 * default. Note that it is still possible to force these functions to
 * return a specific value or check if they have been called using
 * gmock. This makes it easy to convert our existing tests to work
 * against a real Platform and add more introspection in to what we
 * expect the Platform to be doing.
 */
class MockablePlatform : public MockPlatform {
 public:
  explicit MockablePlatform(std::shared_ptr<Platform> realPlatform);
  ~MockablePlatform() override;

 private:
  std::shared_ptr<Platform> realPlatform_{nullptr};

  // Forbidden copy constructor and assignment operator
  MockablePlatform(MockablePlatform const &) = delete;
  MockablePlatform& operator=(MockablePlatform const &) = delete;
};

}} // facebook::fboss
