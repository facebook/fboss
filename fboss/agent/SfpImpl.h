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

#include <cstdint>
#include <folly/String.h>

namespace facebook { namespace fboss {

/*
 * This is class is the SFP implementation class
 */
class SfpImpl {
 public:
  SfpImpl() {}
  virtual ~SfpImpl() {}

  /*
   * Get the SFP EEPROM Field
   */
  virtual int readSfpEeprom(int dataAddress, int offset,
                                          int len, uint8_t* fieldValue) = 0;
  /*
   * This function will check if the SFP is present or not
   */
  virtual bool detectSfp() = 0;
  /*
   * Returns the name of the port
   */
  virtual folly::StringPiece getName() = 0;

 private:
  // Forbidden copy contructor and assignment operator
  SfpImpl(SfpImpl const &) = delete;
  SfpImpl& operator=(SfpImpl const &) = delete;
};

}} // namespace facebook::fboss
