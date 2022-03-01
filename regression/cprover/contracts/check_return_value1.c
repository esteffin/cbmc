int my_function1(void)
  __CPROVER_ensures(__CPROVER_return_value >= 0) // passes
{
  return 10;
}

int my_function2(void)
  __CPROVER_ensures(__CPROVER_return_value >= 0) // fails
{
  return -10;
}
