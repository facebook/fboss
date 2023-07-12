// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/LldpManager.h"
#include "fboss/agent/Main.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/test/AgentTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include <boost/container/flat_set.hpp>

namespace facebook::fboss::utility {

const TransceiverSpec* getTransceiverSpec(const SwSwitch* sw, PortID portId);

} // namespace facebook::fboss::utility
