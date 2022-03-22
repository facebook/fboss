// Copyright 2021-present Facebook. All Rights Reserved.
#include "PollThread.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace std::literals;
using namespace testing;
using namespace rackmon;

struct Tester {
  MOCK_METHOD0(do_stuff, void());
};

TEST(PollThreadTest, basic) {
  Tester test_obj;
  EXPECT_CALL(test_obj, do_stuff()).Times(2);
  PollThread<Tester> thr(&Tester::do_stuff, &test_obj, 1s);
  thr.start();
  // This is a polling function, so we need to sleep to check
  // if the poll happened in the given time.
  // sleep override
  std::this_thread::sleep_for(2s);
  thr.stop();
}

TEST(PollThreadTest, externalEvent) {
  Tester test_obj;
  EXPECT_CALL(test_obj, do_stuff()).Times(2);
  PollThread<Tester> thr(&Tester::do_stuff, &test_obj, 100s);
  thr.start();
  thr.tick();
  thr.stop();
}
