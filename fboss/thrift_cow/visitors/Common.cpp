// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/visitors/Common.h"
#include "folly/Conv.h"

namespace facebook::fboss::thrift_cow {

// Constructors
ThriftTraverseResult::ThriftTraverseResult() : code_(Code::OK) {}

ThriftTraverseResult::ThriftTraverseResult(
    Code code,
    const std::string& message)
    : code_(code), errorMessage_(message) {}

// Implicit conversion to bool for conditional checks
ThriftTraverseResult::operator bool() const {
  return code_ == Code::OK;
}

// Accessors
ThriftTraverseResult::Code ThriftTraverseResult::code() const {
  return code_;
}

std::optional<std::string> ThriftTraverseResult::errorMessage() const {
  return errorMessage_;
}

std::string ThriftTraverseResult::toString() const {
  return folly::to<std::string>(
      "ThriftTraverseResult::",
      codeString(),
      errorMessage_.has_value() ? "(" + errorMessage_.value() + ")" : "");
}

// Comparison operators
bool ThriftTraverseResult::operator==(const ThriftTraverseResult& other) const {
  return code_ == other.code_;
}

bool ThriftTraverseResult::operator!=(const ThriftTraverseResult& other) const {
  return !(*this == other);
}

std::string ThriftTraverseResult::codeString() const {
  switch (code_) {
    case ThriftTraverseResult::Code::OK:
      return "OK";
    case ThriftTraverseResult::Code::NON_EXISTENT_NODE:
      return "NON_EXISTENT_NODE";
    case ThriftTraverseResult::Code::INVALID_ARRAY_INDEX:
      return "INVALID_ARRAY_INDEX";
    case ThriftTraverseResult::Code::INVALID_MAP_KEY:
      return "INVALID_MAP_KEY";
    case ThriftTraverseResult::Code::INVALID_STRUCT_MEMBER:
      return "INVALID_STRUCT_MEMBER";
    case ThriftTraverseResult::Code::INVALID_VARIANT_MEMBER:
      return "INVALID_VARIANT_MEMBER";
    case ThriftTraverseResult::Code::INCORRECT_VARIANT_MEMBER:
      return "INCORRECT_VARIANT_MEMBER";
    case ThriftTraverseResult::Code::VISITOR_EXCEPTION:
      return "VISITOR_EXCEPTION";
    case ThriftTraverseResult::Code::INVALID_SET_MEMBER:
      return "INVALID_SET_MEMBER";
    default:
      return "ThriftTraverseResult::unknown";
  }
}

} // namespace facebook::fboss::thrift_cow
