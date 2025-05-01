# CMake to build libraries and binaries in fboss/lib

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(input_balance_util
  fboss/lib/inputbalance/InputBalanceUtil.cpp
)

target_link_libraries(input_balance_util
  switch_config_cpp2
)
