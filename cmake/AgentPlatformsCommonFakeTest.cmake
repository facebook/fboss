# CMake to build libraries and binaries in fboss/agent/platforms/wedge/wedge400

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fake_test_platform_mapping
  fboss/agent/platforms/common/fake_test/FakeTestPlatformMapping.cpp
)

target_link_libraries(fake_test_platform_mapping
  platform_mapping
)
