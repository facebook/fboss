# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(wdt_service
  fboss/platform/wdt_service.cpp
)

target_link_libraries(wdt_service
  platform_utils
)

