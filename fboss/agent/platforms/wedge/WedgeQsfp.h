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
#include "fboss/agent/TransceiverImpl.h"
#include "fboss/agent/platforms/wedge/WedgeI2CBusLock.h"

namespace facebook { namespace fboss {

/*
 * This is the Wedge Platform Specific Class
 * and contains all the Wedge QSFP Specific Functions
 */
class WedgeQsfp : public TransceiverImpl {
 public:
  WedgeQsfp(int module, WedgeI2CBusLock* i2c);
  virtual ~WedgeQsfp() override;

  /* This function is used to read the SFP EEprom */
  int readTransceiver(int dataAddress, int offset,
                      int len, uint8_t* fieldValue) override;
  /* write to the eeprom (usually to change the page setting) */
  int writeTransceiver(int dataAddress, int offset,
                       int len, uint8_t* fieldValue) override;
  /* This function detects if a SFP is present on the particular port */
  bool detectTransceiver() override;
  /* Returns the name for the port */
  folly::StringPiece getName() override;
  int getNum() override;

 private:
  int module_;
  std::string moduleName_;
  WedgeI2CBusLock* wedgeI2CBusLock_;
};

}} // namespace facebook::fboss
