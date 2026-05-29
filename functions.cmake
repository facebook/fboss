# provide list of libraries  that FBOSS binaries must link with
# because SDK library (libsai_impl.a) uses it and has undefined
# symbols, leading to linker errors while linking binaries.
# specify this list in sdk_dependencies.txt with each library on
# different line (without leading or trailing spaces)
#
#
# $ cat sdk_dependencies.txt
# foo
# bar
# foobar
# foobarfoo
#
#
# FBOSS  binaries link with  above libraries which libsai_impl.a
# needs
function (add_sai_sdk_dependencies name)
  file(READ sdk_dependencies.txt DEPENDENCIES_TEXT)
  string(REPLACE "\n" ";" DEPENDENCIES "${DEPENDENCIES_TEXT}")
  foreach(DEPENDENCY ${DEPENDENCIES})
    target_link_libraries(${name} ${DEPENDENCY})
  endforeach ()

  # Also load sai_dependencies.txt if it exists beside libsai_impl.a.
  # SAI vendors may ship a list of libraries their libsai_impl.a needs;
  # link those too so binaries can resolve the SDK's undefined symbols.
  # Entries may be bare filenames (libfoo.so) that resolve against the
  # SDK's lib dir, -lfoo flags, or absolute paths.
  if (DEFINED SAI_IMPL_DIR AND EXISTS "${SAI_IMPL_DIR}/lib/sai_dependencies.txt")
    file(READ "${SAI_IMPL_DIR}/lib/sai_dependencies.txt" SAI_DEPENDENCIES_TEXT)
    string(REPLACE "\n" ";" SAI_DEPENDENCIES "${SAI_DEPENDENCIES_TEXT}")
    target_link_directories(${name} PRIVATE "${SAI_IMPL_DIR}/lib")
    foreach(DEPENDENCY ${SAI_DEPENDENCIES})
      if (NOT "${DEPENDENCY}" STREQUAL "")
        target_link_libraries(${name} ${DEPENDENCY})
      endif ()
    endforeach ()
  endif ()
endfunction ()

function (strtok str delim out_list)
  set(result_list "")
  string(REPLACE "${delim}" ";" str_list ${str})
  foreach(item IN LISTS str_list)
    list(APPEND result_list "${item}")
  endforeach()
  set(${out_list} "${result_list}" PARENT_SCOPE)
endfunction()

function(BUILD_AND_INSTALL_WITH_XPHY_SDK_LIBS NAME SRCS_VAR DEPS_VAR XPHY_SDK_NAME XPHY_SDK_LIBS_VAR)
  if("${XPHY_SDK_NAME}" STREQUAL "")
    message(STATUS "Building ${NAME} without XPHY SDK")
  else()
    message(STATUS "Building ${NAME} with XPHY SDK=${XPHY_SDK_NAME}")
  endif()

  add_executable(${NAME} ${${SRCS_VAR}})

  target_link_libraries(${NAME} ${${DEPS_VAR}})

  if(NOT "${XPHY_SDK_NAME}" STREQUAL "" AND DEFINED ${XPHY_SDK_LIBS_VAR})
    # Only add XPHY_SDK_LIBS if it's defined and not empty
    target_link_libraries(${NAME}
      -Wl,--whole-archive
      ${${XPHY_SDK_LIBS_VAR}}
      -Wl,--no-whole-archive
    )
  endif()

  install(TARGETS ${NAME})
endfunction()
