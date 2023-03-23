set(SRC bigint-func.cc bigint.cc)

add_library(big-int OBJECT ${SRC})

generic_includes(big-int)
