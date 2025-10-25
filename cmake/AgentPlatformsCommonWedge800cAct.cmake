# CMake to build libraries and binaries in fboss/agent/platforms/common/Wedge800cAct

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge800c_act_platform_mapping
  fboss/agent/platforms/common/Wedge800cAct/Wedge800cActPlatformMapping.cpp
)

target_link_libraries(wedge800c_act_platform_mapping
  platform_mapping
)
