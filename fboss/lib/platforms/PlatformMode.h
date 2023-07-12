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
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"

DECLARE_bool(use_bsp_helpers);

namespace facebook::fboss {

inline std::string toString(PlatformType mode) {
  switch (mode) {
    case PlatformType::PLATFORM_WEDGE:
      return "WEDGE";
    case PlatformType::PLATFORM_WEDGE100:
      return "WEDGE100";
    case PlatformType::PLATFORM_GALAXY_LC:
      return "GALAXY_LC";
    case PlatformType::PLATFORM_GALAXY_FC:
      return "GALAXY_FC";
    case PlatformType::PLATFORM_FAKE_WEDGE:
      return "FAKE_WEDGE";
    case PlatformType::PLATFORM_MINIPACK:
      return "MINIPACK";
    case PlatformType::PLATFORM_YAMP:
      return "YAMP";
    case PlatformType::PLATFORM_FAKE_WEDGE40:
      return "FAKE_WEDGE40";
    case PlatformType::PLATFORM_WEDGE400C:
      return "WEDGE400C";
    case PlatformType::PLATFORM_WEDGE400C_SIM:
      return "WEDGE400C_SIM";
    case PlatformType::PLATFORM_WEDGE400:
      return "WEDGE400";
    case PlatformType::PLATFORM_WEDGE400_GRANDTETON:
      return "WEDGE400_GRANDTETON";
    case PlatformType::PLATFORM_FUJI:
      return "FUJI";
    case PlatformType::PLATFORM_ELBERT:
      return "ELBERT";
    case PlatformType::PLATFORM_CLOUDRIPPER:
      return "CLOUDRIPPER";
    case PlatformType::PLATFORM_DARWIN:
      return "DARWIN";
    case PlatformType::PLATFORM_LASSEN:
      return "LASSEN";
    case PlatformType::PLATFORM_SANDIA:
      return "SANDIA";
    case PlatformType::PLATFORM_MERU400BIU:
      return "MERU400BIU";
    case PlatformType::PLATFORM_MERU400BIA:
      return "MERU400BIA";
    case PlatformType::PLATFORM_MERU400BFU:
      return "MERU400BFU";
    case PlatformType::PLATFORM_WEDGE400C_VOQ:
      return "WEDGE400C_VOQ";
    case PlatformType::PLATFORM_WEDGE400C_FABRIC:
      return "WEDGE400C_FABRIC";
    case PlatformType::PLATFORM_WEDGE400C_GRANDTETON:
      return "WEDGE400C_GRANDTETON";
    case PlatformType::PLATFORM_CLOUDRIPPER_VOQ:
      return "CLOUDRIPPER_VOQ";
    case PlatformType::PLATFORM_CLOUDRIPPER_FABRIC:
      return "CLOUDRIPPER_FABRIC";
    case PlatformType::PLATFORM_MONTBLANC:
      return "MONTBLANC";
    case PlatformType::PLATFORM_MERU800BIA:
      return "MERU800BIA";
    case PlatformType::PLATFORM_MERU800BFA:
      return "MERU800BFA";
    case PlatformType::PLATFORM_MORGAN800CC:
      return "MORGAN800_CC";
  }
  throw std::runtime_error("Unknown mode");
  return "Unknown";
}

enum class ExternalPhyVersion : char {
  MILN4_2,
  MILN5_2,
};
} // namespace facebook::fboss
