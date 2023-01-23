add_library(cbounds SHARED plugin.cpp actions.cpp)
include_directories(include)
target_compile_options(cbounds PUBLIC -fno-exceptions -fno-rtti)
# Force standard to be included in compile_commands.json
target_compile_options(cbounds PUBLIC -std=c++17)

