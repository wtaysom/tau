#include <stdio.h>

typedef struct Where_s {
  const char *file;
  const char *func;
  int line;
} Where_s;

#define WHERE ({ Where_s h = { __FILE__, __FUNCTION__, __LINE__ }; h; })

void PrWhere(Where_s h) {
  printf("%p<%d>%s\n", h.file, h.line, h.func);
}

int main (int argc, char *argv[]) {
  PrWhere(WHERE);
  PrWhere(WHERE);

  return 0;
}
