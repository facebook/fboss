#include <gtest/gtest.h>
#include "folly/init/Init.h"

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  folly::init(&argc, &argv);
  return RUN_ALL_TESTS();
}
