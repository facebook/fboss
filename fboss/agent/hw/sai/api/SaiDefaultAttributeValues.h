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

#include "fboss/agent/hw/sai/api/SaiVersion.h"
#include "fboss/agent/hw/sai/api/Traits.h"
#include "fboss/agent/hw/sai/api/Types.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

struct SaiObjectIdDefault {
  sai_object_id_t operator()() const {
    return SAI_NULL_OBJECT_ID;
  }
};

template <typename SaiIntT>
struct SaiIntDefault {
  SaiIntT operator()() const {
    return 0;
  }
};

struct SaiBoolDefault {
  bool operator()() const {
    return false;
  }
};

struct SaiPortInterfaceTypeDefault {
  sai_port_interface_type_t operator()() const {
    return SAI_PORT_INTERFACE_TYPE_NONE;
  }
};

} // namespace facebook::fboss
