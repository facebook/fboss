/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/fpga/MultiPimPlatformPimContainer.h"

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

std::string MultiPimPlatformPimContainer::getPimTypeStr(
    MultiPimPlatformPimContainer::PimType pimType) {
  switch (pimType) {
    case PimType::MINIPACK_16Q:
      return "MINIPACK_16Q";
    case PimType::MINIPACK_16O:
      return "MINIPACK_16O";
    case PimType::YAMP_16Q:
      return "YAMP_16Q";
    case PimType::FUJI_16Q:
      return "FUJI_16Q";
    case PimType::FUJI_16O:
      return "FUJI_16O";
    case PimType::ELBERT_16Q:
      return "ELBERT_16Q";
    case PimType::ELBERT_8DD:
      return "ELBERT_8DD";
    case PimType::SANDIA_8DD:
      return "SANDIA_8DD";
    case PimType::SANDIA_16Q:
      return "SANDIA_16Q";
  };
  throw FbossError("Unrecognized pimType:", pimType);
}

MultiPimPlatformPimContainer::PimType
MultiPimPlatformPimContainer::getPimTypeFromStr(const std::string& pimTypeStr) {
  if (pimTypeStr == "MINIPACK_16Q") {
    return MultiPimPlatformPimContainer::PimType::MINIPACK_16Q;
  } else if (pimTypeStr == "MINIPACK_16O") {
    return MultiPimPlatformPimContainer::PimType::MINIPACK_16O;
  } else if (pimTypeStr == "YAMP_16Q") {
    return MultiPimPlatformPimContainer::PimType::YAMP_16Q;
  } else if (pimTypeStr == "FUJI_16Q") {
    return MultiPimPlatformPimContainer::PimType::FUJI_16Q;
  } else if (pimTypeStr == "FUJI_16O") {
    return MultiPimPlatformPimContainer::PimType::FUJI_16O;
  } else if (pimTypeStr == "ELBERT_16Q") {
    return MultiPimPlatformPimContainer::PimType::ELBERT_16Q;
  } else if (pimTypeStr == "ELBERT_8DD") {
    return MultiPimPlatformPimContainer::PimType::ELBERT_8DD;
  } else if (pimTypeStr == "SANDIA_8DD") {
    return MultiPimPlatformPimContainer::PimType::SANDIA_8DD;
  } else if (pimTypeStr == "SANDIA_16Q") {
    return MultiPimPlatformPimContainer::PimType::SANDIA_16Q;
  }
  throw FbossError("Unrecognized pimType:", pimTypeStr);
}

MultiPimPlatformPimContainer::MultiPimPlatformPimContainer() {}

MultiPimPlatformPimContainer::~MultiPimPlatformPimContainer() {}
} // namespace facebook::fboss
