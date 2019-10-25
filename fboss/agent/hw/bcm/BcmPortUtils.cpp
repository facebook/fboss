/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmPortUtils.h"

namespace facebook {
namespace fboss {

const PortSpeed2TransmitterTechAndMode& getSpeedToTransmitterTechAndMode() {
  // This allows mapping from a speed and port transmission technology
  // to a broadcom supported interface
  static const PortSpeed2TransmitterTechAndMode kPortTypeMapping = {
      {cfg::PortSpeed::HUNDREDG,
       {{TransmitterTechnology::COPPER, OPENNSL_PORT_IF_CR4},
        {TransmitterTechnology::OPTICAL, OPENNSL_PORT_IF_CAUI},
        // What to default to
        {TransmitterTechnology::UNKNOWN, OPENNSL_PORT_IF_CAUI}}},
      {cfg::PortSpeed::FIFTYG,
       {{TransmitterTechnology::COPPER, OPENNSL_PORT_IF_CR2},
        {TransmitterTechnology::OPTICAL, OPENNSL_PORT_IF_CAUI},
        // What to default to
        {TransmitterTechnology::UNKNOWN, OPENNSL_PORT_IF_CR2}}},
      {cfg::PortSpeed::FORTYG,
       {{TransmitterTechnology::COPPER, OPENNSL_PORT_IF_CR4},
        {TransmitterTechnology::OPTICAL, OPENNSL_PORT_IF_XLAUI},
        // What to default to
        {TransmitterTechnology::UNKNOWN, OPENNSL_PORT_IF_XLAUI}}},
      {cfg::PortSpeed::TWENTYFIVEG,
       {{TransmitterTechnology::COPPER, OPENNSL_PORT_IF_CR},
        {TransmitterTechnology::OPTICAL, OPENNSL_PORT_IF_CAUI},
        // What to default to
        {TransmitterTechnology::UNKNOWN, OPENNSL_PORT_IF_CR}}},
      {cfg::PortSpeed::TWENTYG,
       {{TransmitterTechnology::COPPER, OPENNSL_PORT_IF_CR},
        // We don't expect 20G optics
        // What to default to
        {TransmitterTechnology::UNKNOWN, OPENNSL_PORT_IF_CR}}},
      {cfg::PortSpeed::XG,
       {{TransmitterTechnology::COPPER, OPENNSL_PORT_IF_CR},
        {TransmitterTechnology::OPTICAL, OPENNSL_PORT_IF_SFI},
        // What to default to
        {TransmitterTechnology::UNKNOWN, OPENNSL_PORT_IF_CR}}},
      {cfg::PortSpeed::GIGE,
       {{TransmitterTechnology::COPPER, OPENNSL_PORT_IF_GMII},
        // We don't expect 1G optics
        // What to default to
        {TransmitterTechnology::UNKNOWN, OPENNSL_PORT_IF_GMII}}}};
  return kPortTypeMapping;
}
} // namespace fboss
} // namespace facebook
