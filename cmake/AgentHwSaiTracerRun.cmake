# CMake to build libraries and binaries in fboss/agent/hw/sai/tracer/run

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

function(BUILD_SAI_REPLAYER SAI_IMPL_NAME SAI_IMPL_ARG)

  message(STATUS "Building Sai Replayer SAI_IMPL_NAME: ${SAI_IMPL_NAME} SAI_IMPL_ARG: ${SAI_IMPL_ARG}")

  add_executable(sai_replayer-${SAI_IMPL_NAME}-${SAI_VER_SUFFIX}
    fboss/agent/hw/sai/tracer/run/Main.cpp
    fboss/agent/hw/sai/tracer/run/SaiLog.cpp
  )

  target_link_libraries(sai_replayer-${SAI_IMPL_NAME}-${SAI_VER_SUFFIX}
    # This is needed for 'dlsym', 'dlopen' etc.
    -Wl,--no-as-needed -ldl
    -lz
    ${SAI_IMPL_ARG}
    ${CMAKE_THREAD_LIBS_INIT}
  )

  set_target_properties(sai_replayer-${SAI_IMPL_NAME}-${SAI_VER_SUFFIX}
      PROPERTIES COMPILE_FLAGS
      "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
      -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
      -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
    )

endfunction()

BUILD_SAI_REPLAYER("fake" fake_sai)
install(
  TARGETS
  sai_replayer-fake-${SAI_VER_SUFFIX})

# If libsai_impl is provided, build sai replayer linking with it
find_library(SAI_IMPL sai_impl)
message(STATUS "SAI_IMPL: ${SAI_IMPL}")

if (SAI_BRCM_IMPL)
  find_path(SAI_EXPERIMENTAL_INCLUDE_DIR NAMES saiswitchextensions.h)
  include_directories(${SAI_EXPERIMENTAL_INCLUDE_DIR})
  message(STATUS, "SAI_EXPERIMENTAL_INCLUDE_DIR: ${SAI_EXPERIMENTAL_INCLUDE_DIR}")
endif()

if(SAI_IMPL)
  BUILD_SAI_REPLAYER("sai_impl" ${SAI_IMPL})
  install(
    TARGETS
    sai_replayer-sai_impl-${SAI_VER_SUFFIX})
endif()
