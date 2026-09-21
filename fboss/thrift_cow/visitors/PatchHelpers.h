// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/gen-cpp2/patch_types.h"

#include <string>

#pragma once

namespace facebook::fboss::thrift_cow {

void compressPatch(PatchNode& node);

void decompressPatch(PatchNode& node);

} // namespace facebook::fboss::thrift_cow
