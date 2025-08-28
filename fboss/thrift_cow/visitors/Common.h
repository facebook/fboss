// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <optional>
#include <string>
#include "folly/Conv.h"

namespace facebook::fboss::thrift_cow {

// Class that represents the result of traversing a thrift structure
class ThriftTraverseResult {
 public:
  // Result codes
  enum class Code {
    OK,
    NON_EXISTENT_NODE,
    INVALID_ARRAY_INDEX,
    INVALID_MAP_KEY,
    INVALID_STRUCT_MEMBER,
    INVALID_VARIANT_MEMBER,
    INCORRECT_VARIANT_MEMBER,
    VISITOR_EXCEPTION,
    INVALID_SET_MEMBER,
  };

  // Constructors
  ThriftTraverseResult();
  ThriftTraverseResult(Code code, const std::string& message);

  // Implicit conversion to bool for conditional checks
  explicit operator bool() const;

  // Accessors
  Code code() const;
  std::optional<std::string> errorMessage() const;
  std::string toString() const;

  // Comparison operators
  bool operator==(const ThriftTraverseResult& other) const;
  bool operator!=(const ThriftTraverseResult& other) const;

 private:
  std::string codeString() const;

  Code code_;
  std::optional<std::string> errorMessage_;
};

} // namespace facebook::fboss::thrift_cow
