#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "fboss/platform/helpers/Utils.h"
#include "re2/re2.h"

namespace facebook::fboss::platform::helpers {
TEST(computeExpressionTests, Equal) {
  EXPECT_FLOAT_EQ(computeExpression("x * 0.1", 10.0), 1.0);
  EXPECT_FLOAT_EQ(computeExpression("x * 0.1/100", 10.0), 0.01);
  EXPECT_FLOAT_EQ(computeExpression("(x * 0.1+5)/1000", 100.0, "x"), 0.015);
  EXPECT_FLOAT_EQ(
      computeExpression("(y / 0.1+300)/ (1000*10 + 5)", 30.0, "y"), 0.05997);
  EXPECT_FLOAT_EQ(
      computeExpression("(@ / 0.1+300)/ (1000*10 + 5)", 30.0, "x"), 0.05997);
  EXPECT_FLOAT_EQ(
      computeExpression("(@ / 0.1+300)/ (1000*10 + @ * 10000)", 30.0, "x"),
      0.0019354839);
}
TEST(findFileFromRegexTests, Equal) {
  const std::filesystem::path sandbox{"/tmp/sandbox/"};
  std::filesystem::create_directories(sandbox / "dir1" / "dir2");
  std::filesystem::create_directories(sandbox / "dir1" / "dir2" / "dir3:dir4");
  std::ofstream{sandbox / "dir1" / "file1.txt"};
  std::ofstream{sandbox / "dir1" / "dir2" / "file2.txt"};
  std::ofstream{sandbox / "dir1" / "dir2" / "dir3:dir4" / "file3.txt"};

  EXPECT_EQ(
      findFileFromRegex("/tmp/sandbox/.+/file1.txt"),
      "/tmp/sandbox/dir1/file1.txt");
  EXPECT_EQ(
      findFileFromRegex("/tmp/sandbox/.+/file2.txt"),
      "/tmp/sandbox/dir1/dir2/file2.txt");
  EXPECT_EQ(
      findFileFromRegex("/tmp/sandbox/.+/file3.txt"),
      "/tmp/sandbox/dir1/dir2/dir3:dir4/file3.txt");
  // delete the sandbox dir and all contents within it, including subdirs
  std::filesystem::remove_all(sandbox);
}
} // namespace facebook::fboss::platform::helpers
