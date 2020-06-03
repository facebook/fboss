# CMake to build libraries and binaries in fboss/agent/hw/sai/diag

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(diag_shell
  fboss/agent/hw/sai/diag/DiagShell.cpp
  fboss/agent/hw/sai/diag/oss/DiagShell.cpp
)

target_link_libraries(diag_shell
  sai_repl
  python_repl
  error
  sai_switch
  Folly::folly
  sai_ctrl_cpp2
)

set_target_properties(diag_shell PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

add_library(python_repl
  fboss/agent/hw/sai/diag/PythonRepl.cpp
)

target_link_libraries(python_repl
  error
  Folly::folly
  ${PYTHON_3_7_6}
  # ${PYTHON_3_7_6} requires forkpty and openpty provided by libutil.
  # It is reasonable to assume that /lib64/libutil.so is present and link with
  # it. In future, if needed, we could consider adding a manifest for it.
  util
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

add_executable(diag_shell_client
  fboss/agent/hw/sai/diag/DiagShellClient.cpp
)

target_link_libraries(diag_shell_client
  sai_ctrl_cpp2
  Folly::folly
)

install(TARGETS diag_shell_client)
