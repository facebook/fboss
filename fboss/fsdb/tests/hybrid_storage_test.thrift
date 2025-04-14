// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

namespace cpp2 facebook.fboss.fsdb

cpp_include "folly/container/F14Map.h"

struct TestStruct {
  1: bool tx = false; // inlineBool
  13: map<string, i32> mapOfStringToI32 (allow_skip_thrift_cow = true); // hybridMap
} (thriftpath.root)
