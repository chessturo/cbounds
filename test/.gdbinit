set follow-fork-mode child
dash stack
dash var
define rcbounds
  r --std=c2x -fplugin=../build/bin/libcbounds.so -Xclang -add-plugin -Xclang cbounds $arg0
end
