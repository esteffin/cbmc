int global;

void my_function1(void)
  __CPROVER_assigns(global) // passes
{
  global = 10;
}

void my_function2(int parameter) __CPROVER_assigns() // fails
  __CPROVER_ensures(1)
{
  global = 10;
}

void my_function3(int *pointer)
  __CPROVER_requires(__CPROVER_w_ok(pointer))
  __CPROVER_assigns(*pointer) // passes
{
  *pointer = 10;
}

void my_function4(void) __CPROVER_assigns() // passes
{
  int local;
  local = 10;
}
