# CMake to build libraries and binaries in fboss/lib

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(radix_tree
  fboss/lib/RadixTree.h
  fboss/lib/RadixTree-inl.h
)

target_link_libraries(radix_tree
  Folly::folly
)
