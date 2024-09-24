// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/CowStorage.h"

namespace facebook::fboss::fsdb {

namespace detail {

std::optional<StorageError> parseTraverseResult(
    thrift_cow::ThriftTraverseResult traverseResult) {
  if (traverseResult == thrift_cow::ThriftTraverseResult::OK) {
    return std::nullopt;
  } else if (
      traverseResult == thrift_cow::ThriftTraverseResult::VISITOR_EXCEPTION) {
    XLOG(DBG3) << "Visitor exception on traverse";
    return StorageError::TYPE_ERROR;
  } else {
    XLOG(DBG3) << "Visitor error on traverse: "
               << static_cast<int>(traverseResult);
    return StorageError::INVALID_PATH;
  }
}

std::optional<StorageError> parsePatchResult(
    thrift_cow::PatchApplyResult patchResult) {
  switch (patchResult) {
    case thrift_cow::PatchApplyResult::OK:
      return std::nullopt;
    case thrift_cow::PatchApplyResult::INVALID_STRUCT_MEMBER:
    case thrift_cow::PatchApplyResult::INVALID_VARIANT_MEMBER:
    case thrift_cow::PatchApplyResult::NON_EXISTENT_NODE:
    case thrift_cow::PatchApplyResult::KEY_PARSE_ERROR:
    case thrift_cow::PatchApplyResult::PATCHING_IMMUTABLE_NODE:
      return StorageError::INVALID_PATH;
    case thrift_cow::PatchApplyResult::INVALID_PATCH_TYPE:
      return StorageError::TYPE_ERROR;
  }
  return std::nullopt;
}

} // namespace detail

} // namespace facebook::fboss::fsdb
