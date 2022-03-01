int global;

void my_function1(int parameter)
  __CPROVER_ensures(global >= 0) // passes
{
  global = 10;
}

void my_function2(int parameter)
  __CPROVER_ensures(global >= 0) // fails
{
}
