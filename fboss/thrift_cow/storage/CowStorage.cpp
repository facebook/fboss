// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/CowStorage.h"

namespace facebook::fboss::fsdb {

namespace detail {

std::optional<StorageError> parseTraverseResult(
    const thrift_cow::ThriftTraverseResult& traverseResult) {
  if (traverseResult) {
    return std::nullopt;
  } else if (
      traverseResult.code() ==
      thrift_cow::ThriftTraverseResult::Code::VISITOR_EXCEPTION) {
    XLOG(DBG3) << "Visitor exception on traverse: "
               << traverseResult.toString();
    return StorageError(
        StorageError::Code::TYPE_ERROR, traverseResult.toString());
  } else {
    XLOG(DBG3) << "Visitor error on traverse: " << traverseResult.toString();
    return StorageError(
        StorageError::Code::INVALID_PATH, traverseResult.toString());
  }
}

std::optional<StorageError> parsePatchResult(
    thrift_cow::PatchApplyResult patchResult,
    std::optional<std::pair<
        thrift_cow::pv_detail::PathIter,
        thrift_cow::pv_detail::PathIter>> errorPath) {
  if (patchResult == thrift_cow::PatchApplyResult::OK) {
    return std::nullopt;
  }
  CHECK(errorPath.has_value());
  auto errorPathStr =
      folly::join('/', errorPath.value().first, errorPath.value().second);
  switch (patchResult) {
    case thrift_cow::PatchApplyResult::INVALID_STRUCT_MEMBER:
      return StorageError(
          StorageError::Code::INVALID_PATH,
          "Invalid struct member in patch at path: " + errorPathStr);
    case thrift_cow::PatchApplyResult::INVALID_VARIANT_MEMBER:
      return StorageError(
          StorageError::Code::INVALID_PATH,
          "Invalid variant member in patch at path: " + errorPathStr);
    case thrift_cow::PatchApplyResult::NON_EXISTENT_NODE:
      return StorageError(
          StorageError::Code::INVALID_PATH,
          "Non-existent node in patch at path: " + errorPathStr);
    case thrift_cow::PatchApplyResult::KEY_PARSE_ERROR:
      return StorageError(
          StorageError::Code::INVALID_PATH,
          "Key parse error in patch at path: " + errorPathStr);
    case thrift_cow::PatchApplyResult::PATCHING_IMMUTABLE_NODE:
      return StorageError(
          StorageError::Code::INVALID_PATH,
          "Attempting to patch immutable node at path: " + errorPathStr);
    case thrift_cow::PatchApplyResult::INVALID_PATCH_TYPE:
      return StorageError(
          StorageError::Code::TYPE_ERROR,
          "Invalid patch type at path: " + errorPathStr);
    default:
      return StorageError(
          StorageError::Code::TYPE_ERROR,
          "Unknown patch error at path: " + errorPathStr);
  }
}

} // namespace detail

} // namespace facebook::fboss::fsdb
