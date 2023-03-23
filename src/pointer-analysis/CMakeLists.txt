file(GLOB_RECURSE sources "*.cpp" "*.h")
add_library(pointer-analysis OBJECT ${sources})

generic_includes(pointer-analysis)

target_link_libraries(pointer-analysis util analyses goto-programs)
