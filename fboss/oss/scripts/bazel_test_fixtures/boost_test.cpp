#include <boost/container/vector.hpp>
#include <gtest/gtest.h>
// Exercises a boost container. If libboost_prg_exec_monitor.a (which defines
// its own main()) leaks into the link, this test will fail with
// "duplicate symbol: main" against the main() from //fboss/util/oss:test_main.
TEST(BoostTest, VectorWorks) {
  boost::container::vector<int> v = {1, 2, 3};
  EXPECT_EQ(v.size(), 3u);
}
