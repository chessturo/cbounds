To build
`cmake --build ../build`

To run w/cbounds
`clang-dbg --std=c2x -fplugin=../build/bin/libcbounds.so -Xclang -add-plugin -Xclang cbounds test.c`

Build vendor/llvm
`cd vendor/llvm && cmake -S llvm -B build -DCMAKE_BUILD_TYPE=Debug -G Ninja -DLLVM_USE_LINKER=lld -DLLVM_PARALLEL_LINK_JOBS=1 -DLLVM_ENABLE_PROJECTS=clang`
followed by
`cmake --build build`

Install at `vendor/llvm-install`
`cd build && cmake -DCMAKE_INSTALL_PREFIX=../../llvm-install -P cmake_install.cmake`
