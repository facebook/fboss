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
#include "fboss/qsfp_service/platforms/mlnx/MlnxQsfp.h"
#include "fboss/qsfp_service/platforms/mlnx/MlnxManager.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"
#include "fboss/qsfp_service/sff/QsfpModule.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/qsfp_service/StatsPublisher.h"

#include <folly/String.h>
#include <folly/Format.h>

#include <memory>

extern "C" {
#include <sx/sxd/sxd_access_register.h>
}

using std::make_unique;
using std::unique_ptr;

using folly::sformat;
using folly::Endian;

namespace {
const auto kMaxRegDataSize = 48;
const auto kMaxPageSize = 256;
}

namespace facebook { namespace fboss {

MlnxQsfp::MlnxQsfp(int module) :
  module_(module),
  moduleName_(sformat("QSFP{}", module_)) {
  DLOG(INFO) << "MlnxQsfp created for TransceiverID " << module_;
}

bool MlnxQsfp::detectTransceiver() {
  sxd_status_t rc;
  bool qsfpStatus {false};
  ku_mcia_reg mciaReg;
  sxd_reg_meta_t regMeta;

  std::lock_guard<std::mutex> guard(qsfpEepromLock_);

  mciaReg.module = module_;
  mciaReg.i2c_device_address = TransceiverI2CApi::ADDR_QSFP;
  mciaReg.size = kMaxRegDataSize;
  mciaReg.page_number = 0;

  regMeta.access_cmd = SXD_ACCESS_CMD_GET;

  regMeta.dev_id = MlnxManager::kDefaultDeviceId;
  regMeta.swid = MlnxManager::kDefaultSwid;

  rc = sxd_access_reg_mcia(&mciaReg, &regMeta, 1, nullptr, nullptr);
  mlnxCheckSxdFail(rc, "sxd_access_reg_mcia",
    "Failed to get MCIA register data");

  qsfpStatus = (mciaReg.status == 0); // 0 means GOOD, see PRM

  DLOG(INFO) << "Qsfp status for transceiver ID " << module_ << " "
             << std::boolalpha << qsfpStatus;

  return qsfpStatus;
}

int MlnxQsfp::readTransceiver(int dataAddress, int offset,
                              int len, uint8_t* fieldValue) {
  sxd_status_t rc;
  std::vector<ku_mcia_reg> mciaRegisters;
  std::vector<sxd_reg_meta_t> regMetas;

  DLOG(INFO) << __FUNCTION__ << ": dataAddress " << dataAddress
             << ", offset: " << offset << ", len: " << len;

  // Check input params
  CHECK_LE(offset + len, kMaxPageSize);

  std::lock_guard<std::mutex> guard(qsfpEepromLock_);

  int dataLeft = len;

  do {
    ku_mcia_reg mcia;
    sxd_reg_meta_t regMeta;
    std::memset(&mcia, 0, sizeof(mcia));
    std::memset(&regMeta, 0, sizeof(regMeta));

    // determine how much we can read in one call
    if (dataLeft > kMaxRegDataSize) {
      mcia.size = kMaxRegDataSize;
    } else {
      mcia.size = dataLeft;
    }

    mcia.module = module_;
    mcia.i2c_device_address = dataAddress;
    mcia.device_address = offset + (len - dataLeft);
    mcia.page_number = 0;

    dataLeft -= mcia.size;

    regMeta.access_cmd = SXD_ACCESS_CMD_GET;

    regMeta.dev_id = MlnxManager::kDefaultDeviceId;
    regMeta.swid = MlnxManager::kDefaultSwid;

    // push into vectors
    mciaRegisters.push_back(mcia);
    regMetas.push_back(regMeta);
  } while (dataLeft);

  // read eeprom
  rc = sxd_access_reg_mcia(mciaRegisters.data(),
    regMetas.data(),
    mciaRegisters.size(),
    nullptr /* not used */,
    nullptr /* not used */);
  mlnxCheckSxdFail(rc, "sxd_access_reg_mcia",
    "Failed to get EEPROM data from MCIA register");

  auto* cursor = fieldValue;
  int totalRead = 0;
  // copy to @fieldValue buffer
  for (auto& mciaReg: mciaRegisters) {
    uint32_t* dataStart= &mciaReg.dword_0;

    // convert all dwords in mcia register struct
    // to network byte order
    constexpr int dwordsCount = kMaxRegDataSize/sizeof(mciaReg.dword_0);
    for (int idx = 0; idx < dwordsCount; ++idx) {
     dataStart[idx] = htonl(dataStart[idx]);
    }

    // copy data to fieldBuffer
    std::memcpy(cursor, dataStart, mciaReg.size);

    // move cursor
    cursor += mciaReg.size;

    totalRead += mciaReg.size;
  }

  return totalRead;
}

int MlnxQsfp::writeTransceiver(int dataAddress, int offset,
                               int len, uint8_t* fieldValue) {
  sxd_status_t rc;
  std::vector<ku_mcia_reg> mciaRegisters;
  std::vector<sxd_reg_meta_t> regMetas;

  DLOG(INFO) << __FUNCTION__ << ": dataAddress " << dataAddress
             << ", offset: " << offset << ", len: " << len
             << ", fieldValue: " << (int)(*fieldValue);

  // Check input params
  CHECK_LE(offset + len, kMaxPageSize);

  std::lock_guard<std::mutex> guard(qsfpEepromLock_);

  int dataLeft = len;

  do {
    ku_mcia_reg mcia;
    sxd_reg_meta_t regMeta;
    std::memset(&mcia, 0, sizeof(mcia));
    std::memset(&regMeta, 0, sizeof(regMeta));

    // determine how much we can write in one call
    if (dataLeft > kMaxRegDataSize) {
      mcia.size = kMaxRegDataSize;
    } else {
      mcia.size = dataLeft;
    }

    mcia.module = module_;
    mcia.i2c_device_address = dataAddress;
    mcia.device_address = offset + (len - dataLeft);
    mcia.page_number = 0;

    // copy from fieldsValue
    auto* dataStart = &mcia.dword_0;
    std::memcpy(dataStart, fieldValue + (len - dataLeft), mcia.size);

    // convert all dwords in mcia register struct
    // to host byte order
    constexpr int dwordsCount = kMaxRegDataSize/sizeof(mcia.dword_0);
    for (int idx = 0; idx < dwordsCount; ++idx) {
     dataStart[idx] = ntohl(dataStart[idx]);
    }

    dataLeft -= mcia.size;

    regMeta.access_cmd = SXD_ACCESS_CMD_SET;

    regMeta.dev_id = MlnxManager::kDefaultDeviceId;
    regMeta.swid = MlnxManager::kDefaultSwid;

    // push into vectors
    mciaRegisters.push_back(mcia);
    regMetas.push_back(regMeta);
  } while (dataLeft);

  rc = sxd_access_reg_mcia(mciaRegisters.data(), regMetas.data(),
    mciaRegisters.size(), nullptr, nullptr);
  mlnxCheckSxdFail(rc, "sxd_access_reg_mcia",
    "Failed to write EEPROM data through MCIA register");

  return len - dataLeft;
}

folly::StringPiece MlnxQsfp::getName() {
  return moduleName_;
}

int MlnxQsfp::getNum() const {
  return module_;
}

}}
