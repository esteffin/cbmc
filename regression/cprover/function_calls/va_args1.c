#include <stdarg.h>

int sum(__CPROVER_size_t n, ...)
{
  va_list ap;
  va_start(ap, n);

  int result = 0;

  for(__CPROVER_size_t i = 0; i < n; i++)
  {
    int arg = va_arg(ap, int);
    result += arg;
  }

  return result;
}

int main()
{
  int total;
  total = sum(3, 1, 2, 3);
  __CPROVER_assert(total == 1 + 2 + 3, "property 1");
}
