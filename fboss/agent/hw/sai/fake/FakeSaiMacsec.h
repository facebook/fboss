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
#include "fboss/agent/hw/sai/api/Types.h"
#include "fboss/agent/hw/sai/fake/FakeManager.h"

extern "C" {
#include <sai.h>
#include "fboss/agent/hw/sai/api/fake/saifakeextensions.h"
}

namespace facebook::fboss {

struct FakeMacsec {
  explicit FakeMacsec(sai_macsec_direction_t _direction)
      : direction(_direction) {}
  sai_object_id_t id;
  sai_macsec_direction_t direction;
  bool physicalBypass{false};
};

struct FakeMacsecPort {
  explicit FakeMacsecPort(
      sai_object_id_t _portID,
      sai_macsec_direction_t _macsecDirection)
      : portID(_portID), macsecDirection(_macsecDirection) {}
  sai_object_id_t id;
  sai_object_id_t portID;
  sai_macsec_direction_t macsecDirection;
};

using FakeMacsecManager = FakeManager<sai_object_id_t, FakeMacsec>;
using FakeMacsecPortManager = FakeManager<sai_object_id_t, FakeMacsecPort>;

void populate_macsec_api(sai_macsec_api_t** macsec_api);

} // namespace facebook::fboss
