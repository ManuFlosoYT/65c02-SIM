include(${CMAKE_CURRENT_LIST_DIR}/MinimalNSFRunnerSources.cmake)

add_executable(SIM_65C02 ${MINIMAL_NSF_RUNNER_SOURCES})
configure_minimal_nsf_runner_target(SIM_65C02)

