#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
  int p;
  scanf("Enter an integer: %d\n", &p);
  while (p++) {
    printf("Still negative!\n");
  }

  scanf("Enter an integer: %d\n", &p);
  p += 7;
  {
    int p;
    scanf("Enter an integer: %d\n", &p);
    printf("nested scope!\n");
    int q = 34;
    p--;
    if (p < q) {
      printf("if true!\n");
    } else {
      printf("if false!\n");
    }
  }

  return EXIT_SUCCESS;
}
