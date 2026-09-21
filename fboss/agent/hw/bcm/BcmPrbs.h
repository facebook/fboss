// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <boost/assign.hpp>
#include <boost/bimap.hpp>
#include "fboss/lib/phy/gen-cpp2/prbs_types.h"
extern "C" {
#include <bcm/port.h>
}

namespace facebook::fboss {

using PrbsMap = boost::bimap<int32_t, uint32_t>;
const PrbsMap& asicPrbsPolynomialMap();

prbs::InterfacePrbsState getBcmPortPrbsState(
    int /* unit */,
    bcm_port_t /* port */);

} // namespace facebook::fboss
