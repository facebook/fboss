# CMake to build all FSDB services/executables
# This target ensures all FSDB services/executables are built together
# Useful for CI/CD pipelines to catch breakages in any service/executable
#
# HOW IT WORKS:
# - Each FSDB cmake file that defines an executable appends it to FSDB_EXECUTABLES
# - This file creates a target that depends on all executables in that list
# - New executables are automatically included if they follow the pattern

# Initialize the list if not already done
if(NOT DEFINED FSDB_EXECUTABLES)
  set(FSDB_EXECUTABLES "" CACHE INTERNAL "List of all FSDB executables")
endif()

# Create the target that depends on all FSDB services/executables
add_custom_target(fsdb_all_services)

# Add dependencies dynamically from the collected list
if(FSDB_EXECUTABLES)
  add_dependencies(fsdb_all_services ${FSDB_EXECUTABLES})
  message(STATUS "FSDB all services will build: ${FSDB_EXECUTABLES}")
else()
  message(WARNING "No FSDB executables found in FSDB_EXECUTABLES list")
endif()
