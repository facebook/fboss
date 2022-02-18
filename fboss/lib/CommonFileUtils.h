// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <boost/filesystem/operations.hpp>
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

/*
 * Check whether the given file exists.
 */
bool checkFileExists(const std::string& filename);

boost::filesystem::recursive_directory_iterator recursive_directory_iterator(
    const std::string& path);
} // namespace facebook::fboss
