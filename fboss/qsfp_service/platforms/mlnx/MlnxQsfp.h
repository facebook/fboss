/*
 * Copyright (c) Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "fboss/qsfp_service/sff/TransceiverImpl.h"

#include <mutex>

namespace facebook { namespace fboss {

/*
 * This is the Mlnx Platform Specific Class
 * and contains all the Mlnx QSFP specific Functions
 */
class MlnxQsfp : public TransceiverImpl {
 public:

  /**
   * ctor, constructs MlnxQsfp for transceiver id @module
   *
   * @param module Transceiver id
   */
  MlnxQsfp(int module);

  /**
   * dtor
   */
  ~MlnxQsfp() = default;

  /**
   * This method reads the SFP EEprom
   *
   * @param dataAddress I2C address (for QSFP dataAddress == 0x50)
   * @param offset Offset to read from (0-127)
   * @param len Bytes to read
   * @param filedValue Buffer to store @len bytes of QSFP data
   */
  int readTransceiver(int dataAddress, int offset,
                      int len, uint8_t* fieldValue) override;

  /**
   * This method writes to the eeprom (usually to change the page setting)
   *
   * @param dataAddress I2C address (for QSFP dataAddress == 0x50)
   * @param offset Offset to write to (0-127)
   * @param len Bytes to write
   * @param filedValue Buffer to store @len bytes of QSFP data
   */
  int writeTransceiver(int dataAddress, int offset,
                       int len, uint8_t* fieldValue) override;

  /**
   * This function detects if a SFP is present
   *
   * @ret true if SFP is present, otherwise false
   */
  bool detectTransceiver() override;

  /**
   * Returns the name for the port
   *
   * @ret name for the port
   */
  folly::StringPiece getName() override;

  /**
   * Returns module number (transceiver id)
   *
   * @ret module number
   */
  int getNum() const override;

 private:
  // forbidden copy constructor and assignment operator
  MlnxQsfp(const MlnxQsfp&) = delete;
  MlnxQsfp& operator=(const MlnxQsfp&) = delete;

  // private fields

  // mutex to prevent read and write at the same time
  std::mutex qsfpEepromLock_;

  // module number and module name
  int module_;
  std::string moduleName_;
};

}} // namespace facebook::fboss
