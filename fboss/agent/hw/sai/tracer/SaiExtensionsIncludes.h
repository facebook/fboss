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

// Common SAI extension includes for tracer components.
//
// This header maintains all SAI extension includes to provide a single point of
// management for SAI-related extension headers.

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#if defined(BRCM_SAI_SDK_DNX_GTE_11_0)
#include <experimental/saiexperimentalfirmware.h>
#include <experimental/saiexperimentaltameventaginggroup.h>
#include <saiextensions.h>
#endif
#if defined(BRCM_SAI_SDK_DNX_GTE_12_0)
#include <experimental/saiexperimentalswitchpipeline.h>
#include <experimental/saiexperimentalvendorswitch.h>
#include <saiextensions.h>
#endif
