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

#include <folly/logging/xlog.h>

// Common base macros for transceiver logging. All transceiver log macros
// should build on these to ensure a consistent log format:
//   [prefix]Transceiver:<id> ...
//
// prefix  – optional tag prepended before "Transceiver:" (e.g. "[SM]")
// tcvrId  – any streamable transceiver identifier (name string, int, etc.)
#define TCVR_LOG_BASE(level, prefix, tcvrId) \
  XLOG(level) << prefix << "Transceiver:" << tcvrId << " "

#define TCVR_LOG_BASE_IF(level, cond, prefix, tcvrId) \
  XLOG_IF(level, cond) << prefix << "Transceiver:" << tcvrId << " "
