file(GLOB_RECURSE sources "*.cpp" "*.h")
add_library(linking OBJECT ${sources})

generic_includes(linking)

target_link_libraries(linking util)
