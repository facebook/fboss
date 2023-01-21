# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(ledIO
  fboss/lib/led/LedIO.cpp
)

target_link_libraries(ledIO
  Folly::folly
  led_mapping_cpp2
  error
)
