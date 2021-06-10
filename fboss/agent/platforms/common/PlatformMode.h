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

#include <string>

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
  CLOUDRIPPER
};

inline std::string toString(PlatformMode mode) {
  switch (mode) {
    case PlatformMode::WEDGE:
      return "WEDGE";
    case PlatformMode::WEDGE100:
      return "WEDGE100";
    case PlatformMode::GALAXY_LC:
      return "GALAXY_LC";
    case PlatformMode::GALAXY_FC:
      return "GALAXY_FC";
    case PlatformMode::FAKE_WEDGE:
      return "FAKE_WEDGE";
    case PlatformMode::MINIPACK:
      return "MINIPACK";
    case PlatformMode::YAMP:
      return "YAMP";
    case PlatformMode::FAKE_WEDGE40:
      return "FAKE_WEDGE40";
    case PlatformMode::WEDGE400C:
      return "WEDGE400C";
    case PlatformMode::WEDGE400C_SIM:
      return "WEDGE400C_SIM";
    case PlatformMode::WEDGE400:
      return "WEDGE400";
    case PlatformMode::FUJI:
      return "FUJI";
    case PlatformMode::ELBERT:
      return "ELBERT";
    case PlatformMode::CLOUDRIPPER:
      return "CLOUDRIPPER";
  }
}

enum class ExternalPhyVersion : char {
  MILN4_2,
  MILN5_2,
};
} // namespace facebook::fboss
