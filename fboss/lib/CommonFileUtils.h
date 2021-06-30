// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/Range.h>
#include <string>

namespace facebook::fboss {

/*
 * Remove the given file. Return true if file exists and
 * we were able to remove it, false otherwise
 */
bool removeFile(const std::string& filename);

/*
 * Create given file. Throw if file could not be created else
 * return fd
 */
int createFile(const std::string& filename);

/*
 * Create a directory, recursively creating all parents if necessary.
 *
 * If the directory already exists this function is a no-op.
 * Throws an exception on error.
 */
void createDir(folly::StringPiece path);

} // namespace facebook::fboss
