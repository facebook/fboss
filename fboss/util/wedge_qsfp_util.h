// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/lib/usb/TransceiverPlatformApi.h"

#include <memory>
#include <utility>

std::pair<std::unique_ptr<facebook::fboss::TransceiverI2CApi>, int>
getTransceiverAPI();

/* This function creates and returns the FPGA objects for a platform. This is
 * a utility function which will be called from files like wedge_qsfp_util. The
 * FPGA object created here will be used for testing purpose only. In the
 * regular run thew FPGA object is created by WedgeManager class during
 * initialization
 */
std::pair<std::unique_ptr<facebook::fboss::TransceiverPlatformApi>, int>
getTransceiverPlatformAPI();

bool isTrident2();
