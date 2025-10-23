# CMake to build libraries and binaries in fboss/agent/platforms/common/Wedge800bAct

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge800b_act_platform_mapping
  fboss/agent/platforms/common/Wedge800bAct/Wedge800bActPlatformMapping.cpp
)

target_link_libraries(wedge800b_act_platform_mapping
  platform_mapping
)
