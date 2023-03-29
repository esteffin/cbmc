file(GLOB_RECURSE sources "*.cpp" "*.h")
add_library(goto-programs OBJECT ${sources})

generic_includes(goto-programs)

target_link_libraries(goto-programs assembler json linking)
