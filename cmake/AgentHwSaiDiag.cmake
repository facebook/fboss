# CMake to build libraries and binaries in fboss/agent/hw/sai/diag

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(diag_shell
  fboss/agent/hw/sai/diag/DiagShell.cpp
)

target_link_libraries(diag_shell
  sai_repl
  python_repl
  error
  sai_switch
  Folly::folly
)

set_target_properties(diag_shell PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

add_library(python_repl
  fboss/agent/hw/sai/diag/PythonRepl.cpp
  fboss/agent/hw/sai/diag/oss/PythonRepl.cpp
)

target_link_libraries(python_repl
  error
  Folly::folly
)

add_library(sai_repl
  fboss/agent/hw/sai/diag/SaiRepl.cpp
)

target_link_libraries(sai_repl
  error
  sai_api
  Folly::folly
)

set_target_properties(sai_repl PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)
