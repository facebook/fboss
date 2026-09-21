// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <string>

namespace facebook {
namespace fboss {

/*
 * Get the PIM's ID for a given port name (ie: eth1/2/4 or fab1/23/4)
 */
int getPimID(const std::string& portName);

/*
 * Get the transceiver index for a given port name (ie: eth1/2/4 or fab1/23/4)
 */
int getTransceiverIndexInPim(const std::string& portName);

} // namespace fboss
} // namespace facebook
