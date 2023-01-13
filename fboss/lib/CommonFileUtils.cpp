// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/lib/CommonFileUtils.h"
#include <boost/filesystem/operations.hpp>
#include <folly/logging/xlog.h>
#include <fstream>
#include <sstream>
#include "fboss/agent/SysError.h"

#include <fcntl.h>

namespace facebook::fboss {

bool removeFile(const std::string& filename, bool log) {
  int rv = unlink(filename.c_str());
  if (log) {
    XLOG(INFO) << filename << ((rv == 0) ? " did exist" : " did not exist");
  }

  if (rv == 0) {
    // The file existed and we successfully removed it.
    return true;
  }
  if (errno == ENOENT) {
    // The file wasn't present.
    return false;
  }
  // Some other unexpected error.
  throw SysError(errno, "error while trying to remove file: ", filename);
}

int createFile(const std::string& filename) {
  mode_t mode = S_IRUSR | S_IWUSR | S_IROTH | S_IRGRP;
  auto fd = open(filename.c_str(), O_RDWR | O_CREAT, mode);
  if (fd < 0) {
    // Some other unexpected error.
    throw SysError(errno, "error while trying to create file ", filename);
  }
  return fd;
}

void createDir(folly::StringPiece path) {
  try {
    boost::filesystem::create_directories(path.str());
  } catch (...) {
    throw SysError(errno, "failed to create directory \"", path, "\"");
  }
}

bool checkFileExists(const std::string& filename) {
  // No need to catch exception as this function is BOOST_NOEXCEPT
  return boost::filesystem::exists(filename);
}

boost::filesystem::recursive_directory_iterator recursive_directory_iterator(
    const std::string& path) {
  return boost::filesystem::recursive_directory_iterator(path);
}

std::string readSysfs(const std::string& path) {
  std::ifstream juicejuice(path);
  std::string buf;
  try {
    std::getline(juicejuice, buf);
  } catch (std::exception& e) {
    XLOG(ERR) << "Failed to read sysfs path " << path;
    throw e;
  }
  return buf;
}

bool writeSysfs(const std::string& path, const std::string& val) {
  bool success = true;
  try {
    std::ofstream out(path);
    if (out.is_open()) {
      out << val;
      out.close();
    } else {
      success = false;
    }
  } catch (std::exception& e) {
    success = false;
  }
  return success;
}

} // namespace facebook::fboss
