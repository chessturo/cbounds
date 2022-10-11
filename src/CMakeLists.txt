add_library(cbounds SHARED plugin.cpp)
target_compile_options(cbounds PUBLIC -fno-exceptions -fno-rtti)

