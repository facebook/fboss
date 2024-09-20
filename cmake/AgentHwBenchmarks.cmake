# CMake to build libraries and binaries in fboss/agent/hw/bcm

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(hw_tx_slow_path_rate
  fboss/agent/hw/benchmarks/HwTxSlowPathBenchmark.cpp
)

target_link_libraries(hw_tx_slow_path_rate
  mono_agent_ensemble
  mono_agent_benchmarks
  config_factory
  hw_packet_utils
  ecmp_helper
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_stats_collection_speed
  fboss/agent/hw/benchmarks/HwStatsCollectionBenchmark.cpp
)

target_link_libraries(hw_stats_collection_speed
  config_factory
  hw_packet_utils
  voq_test_utils
  ecmp_helper
  mono_agent_ensemble
  mono_agent_benchmarks
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_fsw_scale_route_add_speed
  fboss/agent/hw/benchmarks/HwFswScaleRouteAddBenchmark.cpp
)

target_link_libraries(hw_fsw_scale_route_add_speed
  mono_agent_ensemble
  mono_agent_benchmarks
  config_factory
  hw_packet_utils
  ecmp_helper
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_fsw_scale_route_del_speed
  fboss/agent/hw/benchmarks/HwFswScaleRouteDelBenchmark.cpp
)

target_link_libraries(hw_fsw_scale_route_del_speed
  mono_agent_ensemble
  mono_agent_benchmarks
  config_factory
  hw_packet_utils
  ecmp_helper
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_th_alpm_scale_route_add_speed
  fboss/agent/hw/benchmarks/HwThAlpmScaleRouteAddBenchmark.cpp
)

target_link_libraries(hw_th_alpm_scale_route_add_speed
  mono_agent_ensemble
  mono_agent_benchmarks
  config_factory
  hw_packet_utils
  ecmp_helper
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_th_alpm_scale_route_del_speed
  fboss/agent/hw/benchmarks/HwThAlpmScaleRouteDelBenchmark.cpp
)

target_link_libraries(hw_th_alpm_scale_route_del_speed
  mono_agent_ensemble
  mono_agent_benchmarks
  config_factory
  hw_packet_utils
  ecmp_helper
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_hgrid_du_scale_route_add_speed
  fboss/agent/hw/benchmarks/HwHgridDUScaleRouteAddBenchmark.cpp
)

target_link_libraries(hw_hgrid_du_scale_route_add_speed
  mono_agent_ensemble
  mono_agent_benchmarks
  config_factory
  hw_packet_utils
  ecmp_helper
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_hgrid_du_scale_route_del_speed
  fboss/agent/hw/benchmarks/HwHgridDUScaleRouteDelBenchmark.cpp
)

target_link_libraries(hw_hgrid_du_scale_route_del_speed
  mono_agent_ensemble
  mono_agent_benchmarks
  config_factory
  hw_packet_utils
  ecmp_helper
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_hgrid_uu_scale_route_add_speed
  fboss/agent/hw/benchmarks/HwHgridUUScaleRouteAddBenchmark.cpp
)

target_link_libraries(hw_hgrid_uu_scale_route_add_speed
  mono_agent_ensemble
  mono_agent_benchmarks
  config_factory
  hw_packet_utils
  ecmp_helper
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_hgrid_uu_scale_route_del_speed
  fboss/agent/hw/benchmarks/HwHgridUUScaleRouteDelBenchmark.cpp
)

target_link_libraries(hw_hgrid_uu_scale_route_del_speed
  mono_agent_ensemble
  mono_agent_benchmarks
  config_factory
  hw_packet_utils
  ecmp_helper
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_rib_resolution_speed
  fboss/agent/hw/benchmarks/HwRibResolutionBenchmark.cpp
)

target_link_libraries(hw_rib_resolution_speed
  config_factory
  mono_agent_ensemble
  mono_agent_benchmarks
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_rib_sync_fib_speed
  fboss/agent/hw/benchmarks/HwRibSyncFibBenchmark.cpp
)

target_link_libraries(hw_rib_sync_fib_speed
  config_factory
  mono_agent_ensemble
  mono_agent_benchmarks
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_teflow_scale_add
  fboss/agent/hw/benchmarks/HwTeFlowScaleAddBenchmark.cpp
)

target_link_libraries(hw_teflow_scale_add
  config_factory
  hw_teflow_utils
  mono_agent_ensemble
  mono_agent_benchmarks
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_teflow_scale_del
  fboss/agent/hw/benchmarks/HwTeFlowScaleDelBenchmark.cpp
)

target_link_libraries(hw_teflow_scale_del
  config_factory
  hw_teflow_utils
  mono_agent_ensemble
  mono_agent_benchmarks
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_teflow_stats_collection_speed
  fboss/agent/hw/benchmarks/HwTeFlowStatsCollectionBenchmark.cpp
)

target_link_libraries(hw_teflow_stats_collection_speed
  config_factory
  hw_teflow_utils
  mono_agent_ensemble
  mono_agent_benchmarks
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_flowlet_stats_collection_speed
  fboss/agent/hw/benchmarks/HwFlowletStatsCollectionBenchmark.cpp
)

target_link_libraries(hw_flowlet_stats_collection_speed
  config_factory
  hw_packet_utils
  ecmp_helper
  load_balancer_utils
  mono_agent_ensemble
  mono_agent_benchmarks
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_ecmp_shrink_speed
  fboss/agent/hw/benchmarks/HwEcmpShrinkSpeedBenchmark.cpp
)

target_link_libraries(hw_ecmp_shrink_speed
  config_factory
  hw_packet_utils
  ecmp_helper
  mono_agent_ensemble
  mono_agent_benchmarks
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_ecmp_shrink_with_competing_route_updates_speed
  fboss/agent/hw/benchmarks/HwEcmpShrinkWithCompetingRouteUpdatesBenchmark.cpp
)

target_link_libraries(hw_ecmp_shrink_with_competing_route_updates_speed
  route_distribution_gen
  config_factory
  hw_packet_utils
  ecmp_helper
  mono_agent_ensemble
  mono_agent_benchmarks
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_rx_slow_path_rate
  fboss/agent/hw/benchmarks/HwRxSlowPathBenchmark.cpp
)

target_link_libraries(hw_rx_slow_path_rate
  config_factory
  copp_test_utils
  hw_qos_utils
  hw_copp_utils
  hw_test_acl_utils
  ecmp_helper
  mono_agent_ensemble
  mono_agent_benchmarks
  Folly::folly
  Folly::follybenchmark
  trap_packet_utils
)

add_library(hw_init_and_exit_benchmark_helper
  fboss/agent/hw/benchmarks/HwInitAndExitBenchmarkHelper.cpp
)

target_link_libraries(hw_init_and_exit_benchmark_helper
  config_factory
  fabric_test_utils
  voq_test_utils
  mono_agent_ensemble
  mono_agent_benchmarks
  route_scale_gen
  prod_config_utils
  Folly::folly
  Folly::follybenchmark
  function_call_time_reporter
)

add_library(hw_init_and_exit_40Gx10G
  fboss/agent/hw/benchmarks/HwInitAndExit40Gx10GBenchmark.cpp
)

target_link_libraries(hw_init_and_exit_40Gx10G
  config_factory
  hw_init_and_exit_benchmark_helper
)

add_library(hw_init_and_exit_100Gx10G
  fboss/agent/hw/benchmarks/HwInitAndExit100Gx10GBenchmark.cpp
)

target_link_libraries(hw_init_and_exit_100Gx10G
  config_factory
  hw_init_and_exit_benchmark_helper
)

add_library(hw_init_and_exit_100Gx25G
  fboss/agent/hw/benchmarks/HwInitAndExit100Gx25GBenchmark.cpp
)

target_link_libraries(hw_init_and_exit_100Gx25G
  config_factory
  hw_init_and_exit_benchmark_helper
)

add_library(hw_init_and_exit_100Gx50G
  fboss/agent/hw/benchmarks/HwInitAndExit100Gx50GBenchmark.cpp
)

target_link_libraries(hw_init_and_exit_100Gx50G
  config_factory
  hw_init_and_exit_benchmark_helper
)

add_library(hw_init_and_exit_100Gx100G
  fboss/agent/hw/benchmarks/HwInitAndExit100Gx100GBenchmark.cpp
)

target_link_libraries(hw_init_and_exit_100Gx100G
  config_factory
  hw_init_and_exit_benchmark_helper
)

add_library(hw_init_and_exit_voq
  fboss/agent/hw/benchmarks/HwInitAndExitVoqBenchmark.cpp
)

target_link_libraries(hw_init_and_exit_voq
  config_factory
  hw_init_and_exit_benchmark_helper
)

add_library(hw_init_and_exit_fabric
  fboss/agent/hw/benchmarks/HwInitAndExitFabricBenchmark.cpp
)

target_link_libraries(hw_init_and_exit_fabric
  config_factory
  hw_init_and_exit_benchmark_helper
)

add_library(hw_anticipated_scale_route_add_speed
  fboss/agent/hw/benchmarks/HwAnticipatedScaleRouteAddBenchmark.cpp
)

target_link_libraries(hw_anticipated_scale_route_add_speed
  config_factory
  hw_packet_utils
  ecmp_helper
  mono_agent_ensemble
  mono_agent_benchmarks
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_anticipated_scale_route_del_speed
  fboss/agent/hw/benchmarks/HwAnticipatedScaleRouteDelBenchmark.cpp
)

target_link_libraries(hw_anticipated_scale_route_del_speed
  config_factory
  hw_packet_utils
  ecmp_helper
  mono_agent_ensemble
  mono_agent_benchmarks
  function_call_time_reporter
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_voq_scale_route_add_speed
  fboss/agent/hw/benchmarks/HwVoqScaleRouteAddBenchmark.cpp
)

target_link_libraries(hw_voq_scale_route_add_speed
  fabric_test_utils
  route_scale_gen
  voq_test_utils
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_voq_scale_route_del_speed
  fboss/agent/hw/benchmarks/HwVoqScaleRouteDelBenchmark.cpp
)

target_link_libraries(hw_voq_scale_route_del_speed
  fabric_test_utils
  route_scale_gen
  voq_test_utils
  Folly::folly
  Folly::follybenchmark
)

add_library(hw_switch_reachability_change_speed
  fboss/agent/hw/benchmarks/HwSwitchReachabilityChangeBenchmark.cpp
)

target_link_libraries(hw_switch_reachability_change_speed
  mono_agent_ensemble
  mono_agent_benchmarks
  config_factory
  voq_test_utils
  fabric_test_utils
  Folly::folly
  Folly::follybenchmark
)
