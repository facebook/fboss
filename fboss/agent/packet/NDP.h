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

namespace facebook { namespace fboss {

/**
 * Constants for NDP option type fields
 *
 * We're using a nested enum inside a class to get "enum class" like syntax,
 * but without the strict type checking, so we can still easily convert to and
 * from uint8_t.
 */
struct NDPOptionType {
  enum Values : uint8_t {
    SRC_LL_ADDRESS = 1,
    TARGET_LL_ADDRESS = 2,
    PREFIX_INFO = 3,
    REDIRECTED_HEADER = 4,
    MTU = 5,
  };
};

/**
 * Length constants for fixed-length NDP option types.
 */
struct NDPOptionLength {
  enum Values : uint8_t {
    SRC_LL_ADDRESS_IEEE802 = 1,
    TARGET_LL_ADDRESS_IEEE802 = 1,
    PREFIX_INFO = 4,
    // REDIRECTED_HEADER is variable length
    MTU = 1,
  };
};

}} // facebook::fboss
