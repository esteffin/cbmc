file(GLOB_RECURSE sources "*.cpp" "*.h")
add_executable(run-symex-only ${sources})

generic_includes(run-symex-only)

target_link_libraries(run-symex-only cprover-api-cpp)
