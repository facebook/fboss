/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <folly/FBString.h>
#include <folly/IPAddress.h>
#include <folly/dynamic.h>
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::rib {

/**
 * Route forward actions
 */
using RouteForwardAction = facebook::fboss::RouteForwardAction;

/**
 * Route prefix
 */
template <typename AddrT>
using RoutePrefix = facebook::fboss::RoutePrefix<AddrT>;

using PrefixV4 = facebook::fboss::RoutePrefixV4;
using PrefixV6 = facebook::fboss::RoutePrefixV6;
using NextHopWeight = facebook::fboss::NextHopWeight;

using NextHop = facebook::fboss::NextHop;
using ResolvedNextHop = facebook::fboss::ResolvedNextHop;

using UnresolvedNextHop = facebook::fboss::UnresolvedNextHop;

} // namespace facebook::fboss::rib
