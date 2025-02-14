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
endfunction ()

function (strtok str delim out_list)
  set(result_list "")
  string(REPLACE "${delim}" ";" str_list ${str})
  foreach(item IN LISTS str_list)
    list(APPEND result_list "${item}")
  endforeach()
  set(${out_list} "${result_list}" PARENT_SCOPE)
endfunction()
