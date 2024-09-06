# CMake to build libraries and binaries in fboss/agent/hw/sai/benchmarks

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

# NOTE: All the benchmark executables need to link in ${SAI_IMPL_ARG}
# using '--whole-archive' flag in order to ensure SAI_IMPL symbols are included

function(BUILD_SAI_BENCHMARKS SAI_IMPL_NAME SAI_IMPL_ARG)

  message(STATUS "Building SAI benchmarks SAI_IMPL_NAME: ${SAI_IMPL_NAME} SAI_IMPL_ARG: ${SAI_IMPL_ARG}")

  add_executable(sai_fsw_scale_route_add_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_fsw_scale_route_add_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_fsw_scale_route_add_speed
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_fsw_scale_route_add_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_fsw_scale_route_del_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_fsw_scale_route_del_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_fsw_scale_route_del_speed
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_fsw_scale_route_del_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_th_alpm_scale_route_add_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_th_alpm_scale_route_add_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_th_alpm_scale_route_add_speed
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_th_alpm_scale_route_add_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_th_alpm_scale_route_del_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_th_alpm_scale_route_del_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_th_alpm_scale_route_del_speed
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_th_alpm_scale_route_del_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_hgrid_du_scale_route_add_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_hgrid_du_scale_route_add_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    hw_hgrid_du_scale_route_add_speed
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_hgrid_du_scale_route_add_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_hgrid_du_scale_route_del_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_hgrid_du_scale_route_del_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_hgrid_du_scale_route_del_speed
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_hgrid_du_scale_route_del_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_hgrid_uu_scale_route_add_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_hgrid_uu_scale_route_add_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_hgrid_uu_scale_route_add_speed
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_hgrid_uu_scale_route_add_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_hgrid_uu_scale_route_del_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_hgrid_uu_scale_route_del_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_hgrid_uu_scale_route_del_speed
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_hgrid_uu_scale_route_del_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_anticipated_scale_route_add_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_anticipated_scale_route_add_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_anticipated_scale_route_add_speed
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_anticipated_scale_route_add_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_anticipated_scale_route_del_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_anticipated_scale_route_del_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_anticipated_scale_route_del_speed
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_anticipated_scale_route_del_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )
  add_executable(sai_stats_collection_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_stats_collection_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    hw_stats_collection_speed
    mono_sai_agent_benchmarks_main
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_stats_collection_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_tx_slow_path_rate-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_tx_slow_path_rate-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    hw_tx_slow_path_rate
    mono_sai_agent_benchmarks_main
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_tx_slow_path_rate-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_ecmp_shrink_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_ecmp_shrink_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    hw_ecmp_shrink_speed
    mono_sai_agent_benchmarks_main
    sai_ecmp_utils
    sai_port_utils
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_ecmp_shrink_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_ecmp_shrink_with_competing_route_updates_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_ecmp_shrink_with_competing_route_updates_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_ecmp_shrink_with_competing_route_updates_speed
    sai_ecmp_utils
    sai_port_utils
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_ecmp_shrink_with_competing_route_updates_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_rx_slow_path_rate-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_rx_slow_path_rate-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    mono_sai_agent_benchmarks_main
    hw_rx_slow_path_rate
    sai_copp_utils
    sai_acl_utils
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_rx_slow_path_rate-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_init_and_exit_40Gx10G-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_init_and_exit_40Gx10G-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    sai_copp_utils
    hw_init_and_exit_40Gx10G
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_init_and_exit_40Gx10G-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_init_and_exit_100Gx10G-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_init_and_exit_100Gx10G-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    sai_copp_utils
    hw_init_and_exit_100Gx10G
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_init_and_exit_100Gx10G-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_init_and_exit_100Gx25G-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_init_and_exit_100Gx25G-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    sai_copp_utils
    hw_init_and_exit_100Gx25G
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_init_and_exit_100Gx25G-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_init_and_exit_100Gx50G-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_init_and_exit_100Gx50G-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    sai_copp_utils
    hw_init_and_exit_100Gx50G
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_init_and_exit_100Gx50G-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_init_and_exit_100Gx100G-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_init_and_exit_100Gx100G-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    sai_copp_utils
    hw_init_and_exit_100Gx100G
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_init_and_exit_100Gx100G-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_init_and_exit_voq-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_init_and_exit_voq-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    sai_copp_utils
    hw_init_and_exit_voq
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_init_and_exit_voq-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_init_and_exit_fabric-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_init_and_exit_fabric-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    sai_copp_utils
    hw_init_and_exit_fabric
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_init_and_exit_fabric-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_rib_resolution_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_rib_resolution_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    hw_rib_resolution_speed
    mono_sai_agent_benchmarks_main
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_rib_resolution_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_rib_sync_fib_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_rib_sync_fib_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    hw_rib_sync_fib_speed
    mono_sai_agent_benchmarks_main
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_rib_sync_fib_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_voq_scale_route_add_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_voq_scale_route_add_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    hw_voq_scale_route_add_speed
    mono_sai_agent_benchmarks_main
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_voq_scale_route_add_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_voq_scale_route_del_speed-${SAI_IMPL_NAME} /dev/null)

  target_link_libraries(sai_voq_scale_route_del_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    hw_voq_scale_route_del_speed
    mono_sai_agent_benchmarks_main
    route_scale_gen
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

  set_target_properties(sai_voq_scale_route_del_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

  add_executable(sai_switch_reachability_change_speed-${SAI_IMPL_NAME} /dev/null)
  
  target_link_libraries(sai_switch_reachability_change_speed-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    hw_switch_reachability_change_speed
    mono_sai_agent_benchmarks_main
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  ) 
  
  set_target_properties(sai_switch_reachability_change_speed-${SAI_IMPL_NAME}
    PROPERTIES COMPILE_FLAGS
    "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
    -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
    -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
  )

endfunction()

if(BUILD_SAI_FAKE AND BUILD_SAI_FAKE_BENCHMARKS)
  BUILD_SAI_BENCHMARKS("fake" fake_sai)
endif()

# If libsai_impl is provided, build sai tests linking with it
find_library(SAI_IMPL sai_impl)
message(STATUS "SAI_IMPL: ${SAI_IMPL}")

if (SAI_BRCM_IMPL)
  find_path(SAI_EXPERIMENTAL_INCLUDE_DIR NAMES saiswitchextensions.h)
  include_directories(${SAI_EXPERIMENTAL_INCLUDE_DIR})
  message(STATUS, "SAI_EXPERIMENTAL_INCLUDE_DIR: ${SAI_EXPERIMENTAL_INCLUDE_DIR}")
endif()

if(SAI_IMPL AND BENCHMARK_INSTALL)
  BUILD_SAI_BENCHMARKS("sai_impl" ${SAI_IMPL})

  install(
    TARGETS
    sai_fsw_scale_route_add_speed-sai_impl)
  install(
    TARGETS
    sai_hgrid_du_scale_route_add_speed-sai_impl)
  install(
    TARGETS
    sai_hgrid_uu_scale_route_del_speed-sai_impl)
  install(
    TARGETS
    sai_th_alpm_scale_route_add_speed-sai_impl)
  install(
    TARGETS
    sai_fsw_scale_route_del_speed-sai_impl)
  install(
    TARGETS
    sai_ecmp_shrink_with_competing_route_updates_speed-sai_impl)
  install(
    TARGETS
    sai_th_alpm_scale_route_del_speed-sai_impl)
  install(
    TARGETS
    sai_ecmp_shrink_speed-sai_impl)
  install(
    TARGETS
    sai_hgrid_uu_scale_route_add_speed-sai_impl)
  install(
    TARGETS
    sai_hgrid_du_scale_route_del_speed-sai_impl)
  install(
    TARGETS
    sai_stats_collection_speed-sai_impl)
  install(
    TARGETS
    sai_tx_slow_path_rate-sai_impl)
  install(
    TARGETS
    sai_rx_slow_path_rate-sai_impl)
  install(
    TARGETS
    sai_init_and_exit_40Gx10G-sai_impl)
  install(
    TARGETS
    sai_init_and_exit_100Gx10G-sai_impl)
  install(
    TARGETS
    sai_init_and_exit_100Gx25G-sai_impl)
  install(
    TARGETS
    sai_init_and_exit_100Gx50G-sai_impl)
  install(
    TARGETS
    sai_init_and_exit_100Gx100G-sai_impl)
  install(
    TARGETS
    sai_rib_resolution_speed-sai_impl)
  install(
    TARGETS
    sai_switch_reachability_change_speed-sai_impl)
endif()
