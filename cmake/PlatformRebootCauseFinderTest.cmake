# Make to build libraries and binaries in fboss/platform/reboot_cause_finder

add_executable(reboot_cause_finder_impl_test
  fboss/platform/reboot_cause_finder/tests/RebootCauseFinderImplTest.cpp
)

target_link_libraries(reboot_cause_finder_impl_test
  reboot_cause_finder_lib
  fmt::fmt
  Folly::folly
  FBThrift::thriftcpp2
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(reboot_cause_finder_impl_test)
