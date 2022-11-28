int main(int argc, char *argv[]) {
  char str[] = { 's', 't', 'r', '\0' };

  // Works fine
  char *p = str;
  while (*p != '\0') p++;
  int str_len = p - str;

  // No longer NUL terminated!
  str[3] = 's';

  // Overread
  p = str;
  while (*p != '\0') p++;
  int broken_str_len = p - str;

  return 0;
}
