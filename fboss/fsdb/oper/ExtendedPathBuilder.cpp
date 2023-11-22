/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include <folly/Conv.h>

namespace facebook::fboss::fsdb {

ExtendedOperPath ExtendedPathBuilder::get() const& {
  return path;
}

ExtendedOperPath&& ExtendedPathBuilder::get() && {
  return std::move(path);
}

ExtendedPathBuilder& ExtendedPathBuilder::raw(std::string token) {
  OperPathElem elem;
  elem.raw_ref() = token;
  path.path()->emplace_back(std::move(elem));
  return *this;
}

ExtendedPathBuilder& ExtendedPathBuilder::raw(apache::thrift::field_id_t id) {
  return raw(folly::to<std::string>(id));
}

ExtendedPathBuilder& ExtendedPathBuilder::regex(std::string regexStr) {
  // validate inline?
  OperPathElem elem;
  elem.regex_ref() = regexStr;
  path.path()->emplace_back(std::move(elem));
  return *this;
}

ExtendedPathBuilder& ExtendedPathBuilder::any() {
  // validate inline?
  OperPathElem elem;
  elem.any_ref() = true;
  path.path()->emplace_back(std::move(elem));
  return *this;
}

namespace ext_path_builder {
ExtendedPathBuilder raw(std::string token) {
  return ExtendedPathBuilder().raw(token);
}

ExtendedPathBuilder raw(apache::thrift::field_id_t id) {
  return raw(folly::to<std::string>(id));
}

ExtendedPathBuilder regex(std::string regexStr) {
  return ExtendedPathBuilder().regex(regexStr);
}

ExtendedPathBuilder any() {
  return ExtendedPathBuilder().any();
}
} // namespace ext_path_builder

} // namespace facebook::fboss::fsdb
