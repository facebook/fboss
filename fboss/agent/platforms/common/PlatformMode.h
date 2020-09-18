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

enum class PlatformMode : char {
  WEDGE,
  WEDGE100,
  GALAXY_LC,
  GALAXY_FC,
  FAKE_WEDGE,
  MINIPACK,
  YAMP,
  FAKE_WEDGE40,
  WEDGE400C,
  WEDGE400C_SIM,
  WEDGE400,
  FUJI,
  ELBERT,
};

enum class ExternalPhyVersion : char {
  MILN4_2,
  MILN5_2,
};
} // namespace facebook::fboss
