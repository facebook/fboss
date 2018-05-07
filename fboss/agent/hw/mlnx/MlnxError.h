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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/SysError.h"

extern "C" {
#include <sx/sdk/sx_status.h>
}

#include <folly/Format.h>

namespace facebook { namespace fboss {

/**
 * A class for MLNX SDK errors
 */
class MlnxError : public FbossError {
 public:
  template<typename... Args>
  MlnxError(sx_status_t rc, const std::string& funcName, Args&&... args) :
    FbossError(funcName, "(): ",
      std::forward<Args>(args)...,
      ": ",
      SX_STATUS_MSG(rc)),
    rc_(static_cast<int>(rc)) {}

  template<typename... Args>
  MlnxError(sxd_status_t rc, const std::string& funcName, Args&&... args) :
    FbossError(funcName, "(): ", std::forward<Args>(args)..., ": ",
      SX_STATUS_MSG(sxd_status_to_sx_status(rc))),
    rc_(static_cast<int>(rc)) {}

  template<typename... Args>
  MlnxError(int rc, const std::string& funcName, Args&&... args) :
    FbossError(funcName , "(): ", std::forward<Args>(args)..., ": ", rc),
    rc_(rc) {}

  virtual ~MlnxError() override = default;

  int getRc() const {
    return rc_;
  }

 private:
  int rc_;
};

/**
 * Check for sdk error.
 * If error occurred throws an MlnxError
 * with specifying error by rc
 *
 * @param rc Return code
 * @param msgArgs additional arguments for MlnxError
 */
template<typename... Args>
void mlnxCheckSdkFail(sx_status_t rc,
  const std::string& funcName,
  Args&&... msgArgs) {
  if (rc != SX_STATUS_SUCCESS) {
    throw MlnxError(rc, funcName, std::forward<Args>(msgArgs)...);
  }
}

/**
 * Check for error from sxd layer.
 * If error occurred throws an MlnxError
 * with specifying error by rc value
 *
 * @param rc sxd return code
 * @param msgArgs additional arguments for MlnxError
 */
template<typename... Args>
void mlnxCheckSxdFail(sxd_status_t rc,
  const std::string& funcName,
  Args&&... msgArgs) {
  if (rc != SXD_STATUS_SUCCESS) {
    throw MlnxError(rc, funcName, std::forward<Args>(msgArgs)...);
  }
}

/**
 *
 * Check for system error.
 * If error occurred throws an SysError
 * with specifying error by errno value
 *
 * @param rc Return code
 * @param msgArgs additional arguments for SysError
 */
template<typename... Args>
void mlnxCheckFail(int rc, Args&&... msgArgs) {
  if (rc != 0) {
    throw SysError(rc, std::forward<Args>(msgArgs)...);
  }
}

/**
 * Check rc from SX api.
 * If error occurred logs an error
 *
 * @param rc SX return code
 * @param funcName SX API function name
 * @param msgArgs additional error message strings
 *
 */
template<typename... Args>
void mlnxLogSxError(sx_status_t rc,
  const std::string& funcName,
  Args&&... msgArgs) {
  if (rc != SX_STATUS_SUCCESS) {
    LOG(ERROR) << funcName << "(): "
               << folly::to<std::string>(std::forward<Args>(msgArgs)...)
               << ": " << (SX_STATUS_MSG(rc));
  }
}

}}

