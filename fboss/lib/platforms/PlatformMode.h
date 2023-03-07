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

#include <gflags/gflags.h>
#include <stdexcept>
#include <string>

DECLARE_bool(use_bsp_helpers);

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
  CLOUDRIPPER,
  DARWIN,
  LASSEN,
  SANDIA,
  MERU400BIU,
  MERU400BIA,
  MERU400BFU,
  WEDGE400C_VOQ,
  WEDGE400C_FABRIC,
  CLOUDRIPPER_VOQ,
  CLOUDRIPPER_FABRIC,
  MONTBLANC,
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
    case PlatformMode::DARWIN:
      return "DARWIN";
    case PlatformMode::LASSEN:
      return "LASSEN";
    case PlatformMode::SANDIA:
      return "SANDIA";
    case PlatformMode::MERU400BIU:
      return "MERU400BIU";
    case PlatformMode::MERU400BIA:
      return "MERU400BIA";
    case PlatformMode::MERU400BFU:
      return "MERU400BFU";
    case PlatformMode::WEDGE400C_VOQ:
      return "WEDGE400C_VOQ";
    case PlatformMode::WEDGE400C_FABRIC:
      return "WEDGE400C_FABRIC";
    case PlatformMode::CLOUDRIPPER_VOQ:
      return "CLOUDRIPPER_VOQ";
    case PlatformMode::CLOUDRIPPER_FABRIC:
      return "CLOUDRIPPER_FABRIC";
    case PlatformMode::MONTBLANC:
      return "MONTBLANC";
  }
  throw std::runtime_error("Unknown mode");
  return "Unknown";
}

enum class ExternalPhyVersion : char {
  MILN4_2,
  MILN5_2,
};
} // namespace facebook::fboss
