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
/*
 * Base class for Hardware Abstraction Layer(HAL).
 */
class BcmHALVector {
 public:
  BcmHALVector() {}

  virtual ~BcmHALVector() {}

  virtual void initialize() = 0;

 protected:
  virtual void initializeIO() = 0;
  virtual void initializeDMA() = 0;
  virtual void initializeInterrupt() = 0;

 private:
  // Forbidden copy constructor and assignment operator
  BcmHALVector(BcmHALVector const&) = delete;
  BcmHALVector& operator=(BcmHALVector const&) = delete;
};

} // namespace facebook::fboss
