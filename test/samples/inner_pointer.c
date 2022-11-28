int main([[clang::annotate("length_of", "argv")]] int argc, char *argv[]) {
  int x = 3;
  int y = -4;
  int v = y;
  v = x;

  int q[] = {1, -2}; // q : 0 -> 1
  int *f = q;        // f : 0 -> 1
  f++;               // f : -1 -> 0
  int *j = f + 3;    // j : -4 -> -3
  return *argv[0] == 't';
}

