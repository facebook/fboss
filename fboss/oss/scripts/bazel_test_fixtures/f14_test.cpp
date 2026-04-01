#include <folly/container/F14Map.h>
#include <gtest/gtest.h>
// Exercises folly's F14 containers. If the F14 detail archive is not
// force-linked the F14LinkCheck symbol is dropped and linking fails.
TEST(FollyF14, Works) {
  folly::F14FastMap<int, int> m;
  m[1] = 2;
  EXPECT_EQ(m[1], 2);
}
