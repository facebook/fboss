# CMake to build libraries and binaries in fboss/agent/platforms/common/meru800bia

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(meru800bfa_platform_mapping
  fboss/agent/platforms/common/meru800bfa/Meru800bfaPlatformMapping.cpp
)

target_link_libraries(meru800bfa_platform_mapping
  platform_mapping
)
