#include <stdlib.h>

int main()
{
  int *p;
  p = malloc(1);
  void **q = &p;
  void *new_mem = malloc(1);
  free(*q);
}
