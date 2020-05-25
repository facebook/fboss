# CMake to build libraries and binaries in fboss/agent/hw/sai/store

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(sai_store
  fboss/agent/hw/sai/store/SaiObjectEventPublisher.cpp
  fboss/agent/hw/sai/store/SaiObject.cpp
  fboss/agent/hw/sai/store/SaiStore.cpp
)

target_link_libraries(sai_store
  fake_sai
  sai_api
  ref_map
  tuple_utils
)

set_target_properties(sai_store PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)
