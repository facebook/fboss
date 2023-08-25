// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#ifndef IS_OSS
#include "fboss/fsdb/if/facebook/gen-cpp2-thriftpath/fsdb_model.h" // @manual=//fboss/fsdb/if/facebook:fsdb_model-cpp2-thriftpath
#include "fboss/fsdb/if/facebook/gen-cpp2/fsdb_model_fatal_types.h"
#include "fboss/fsdb/if/facebook/gen-cpp2/fsdb_model_types.h"
#else
// file manually generated and copied to repo
#include "fboss/fsdb/if/oss/fsdb_model_thriftpath.h" // @manual
#include "fboss/fsdb/if/oss/gen-cpp2/fsdb_model_fatal_types.h" // @manual
#include "fboss/fsdb/if/oss/gen-cpp2/fsdb_model_types.h" // @manual
#endif
