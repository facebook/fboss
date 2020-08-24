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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

// The run_tracer function in SaiLog.cpp will be replaced by the generated
// code captured by sai tracer. It will auto generate API calls and
// initialization steps to be replayed.

void run_trace();

} // namespace facebook::fboss
