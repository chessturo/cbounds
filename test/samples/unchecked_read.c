#include <stdio.h>
#include <stdlib.h>
#define SIZE_STEP 16

int main(int argc, char *argv[]) {
  char *str, ch;
  size_t len;
  size_t str_size;

  printf("Enter some bytes: ");
  str_size = SIZE_STEP * sizeof(*str);
  str = (char *) malloc(str_size);
  len = 0;

  // Read from stdin until EOF or \n
  while ((ch = fgetc(stdin)) != EOF && ch != '\n') {
    // Read next char
    str[len] = ch;
    len++;

    // Resize buffer
    if (len == str_size) {
      str_size += SIZE_STEP * sizeof(*str);
      str = (char *) realloc(str, str_size);

      // If realloc fails, quit
      if (str == NULL) return EXIT_FAILURE;
    }
  }
  // Add the nul terminator
  str[len] = '\0';

  printf("Number of bytes entered: ");
  // Bug is here, len gets overwritten because we pass a non-const pointer to it
  if (scanf("%zu",  &len) != 1) return EXIT_FAILURE;
  for (int i = 0; i < len; i++) printf("%c", str[i]);

  free(str);
  return EXIT_SUCCESS;
}

