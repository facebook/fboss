# CMake to build libraries and binaries in fboss/agent/platforms/common/janga

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(janga800bic_platform_mapping
  fboss/agent/platforms/common/janga800bic/Janga800bicPlatformMapping.cpp
)

target_link_libraries(janga800bic_platform_mapping
  platform_mapping
)
