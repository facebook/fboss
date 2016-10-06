/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#pragma once

#include "fboss/agent/FbossError.h"

namespace facebook { namespace fboss {

class SaiError : public FbossError {
public:
  template<typename... Args>
  explicit SaiError(Args&&... args)
  : FbossError("SAI-agent: ", std::forward<Args>(args)...){
  }

  ~SaiError() throw() {}
};

class SaiFatal : public FbossError {
public:
  template<typename... Args>
  explicit SaiFatal(Args&&... args)
  : FbossError("SAI-agent: ", std::forward<Args>(args)...){
  }

  ~SaiFatal() throw() {}
};

}} // facebook::fboss
