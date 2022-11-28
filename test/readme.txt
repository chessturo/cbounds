To build
`cmake --build ../build`

To run w/cbounds
`clang-15 --std=c2x -fplugin=../build/bin/libcbounds.so -Xclang -add-plugin -Xclang cbounds test.c`
