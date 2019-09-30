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

#include <folly/String.h>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

folly::StringPiece saiApiTypeToString(sai_api_t apiType);
folly::StringPiece saiObjectTypeToString(sai_object_type_t objectType);

} // namespace facebook::fboss
