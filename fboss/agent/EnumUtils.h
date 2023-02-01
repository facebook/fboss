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
#include <thrift/lib/cpp/util/EnumUtils.h>

namespace facebook::fboss {
template <typename Enum>
std::string enumToName(Enum enumInput) {
  auto name = apache::thrift::util::enumName(enumInput);
  if (name == nullptr) {
    XLOG(FATAL) << "Unexpected enum: " << static_cast<int>(enumInput);
  }
  return name;
}

template <typename Enum>
Enum nameToEnum(const std::string& valueAsString) {
  Enum enumOutput;
  if (!apache::thrift::TEnumTraits<Enum>::findValue(
          valueAsString, &enumOutput)) {
    XLOG(FATAL) << "Invalid enum value as string: " << valueAsString;
  }
  return enumOutput;
}
} // namespace facebook::fboss
