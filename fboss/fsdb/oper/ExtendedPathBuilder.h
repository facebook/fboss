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

#include <thrift/lib/cpp2/reflection/reflection.h>
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::fsdb {

class ExtendedPathBuilder {
  // TODO: integrate with ThriftPath plugin to provide more of a typed
  // builder model.
 public:
  ExtendedOperPath get() const&;
  ExtendedOperPath&& get() &&;

  ExtendedPathBuilder& raw(std::string token);
  ExtendedPathBuilder& raw(apache::thrift::field_id_t id);
  ExtendedPathBuilder& regex(std::string regexStr);
  ExtendedPathBuilder& any();

 private:
  ExtendedOperPath path;
};

namespace ext_path_builder {

// helpers to create a single element builder
ExtendedPathBuilder raw(std::string token);
ExtendedPathBuilder raw(apache::thrift::field_id_t id);
ExtendedPathBuilder regex(std::string regexStr);
ExtendedPathBuilder any();

} // namespace ext_path_builder

} // namespace facebook::fboss::fsdb
