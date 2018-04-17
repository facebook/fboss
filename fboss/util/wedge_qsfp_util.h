// Copyright 2004-present Facebook. All Rights Reserved.
#pragma once

#include "fboss/lib/usb/TransceiverI2CApi.h"

#include <memory>
#include <utility>

std::pair<std::unique_ptr<facebook::fboss::TransceiverI2CApi>, int>
getTransceiverAPI();

bool isTrident2();
