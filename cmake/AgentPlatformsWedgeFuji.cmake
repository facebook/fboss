# CMake to build libraries and binaries in fboss/agent/platforms/wedge/fuji

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fuji_platform_mapping
  fboss/agent/platforms/wedge/fuji/FujiPlatformMapping.cpp
  fboss/agent/platforms/wedge/fuji/Fuji16QPimPlatformMapping.cpp
)

target_link_libraries(fuji_platform_mapping
  platform_mapping
)
